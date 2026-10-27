"""Compile actual dispatcher/PCM methods with device-free radio/buffer doubles.
Checks PTT release on start failure, cancellation, backpressure and stereo packing.
No sound card, radio or RF transmission is used.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]


def method(path, signature):
    text = (root / path).read_text(encoding='utf-8-sig')
    start = text.index(signature)
    opening = text.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


prefix = r'''
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>
using quint32=uint32_t;using quint16=uint16_t;using qint16=int16_t;
#define SOUNDFRAME quint32
#define TXSTRIPE 1024
#define TONEBUFLEN 40
#define emit
std::vector<std::string> events;
bool swapChannel=false,pttToneOtherChannel=false,timeoutNow=false;
std::atomic_bool *cancelOnWait=nullptr;
void usleep(int){if(cancelOnWait)cancelOnWait->store(true);}
class QElapsedTimer {public:void start(){}int elapsed()const{return timeoutNow?6001:0;}};
template<class T>T qBound(T low,T value,T high){return std::min(high,std::max(low,value));}
struct Buffer {
 bool full=false;int attempts=0;std::vector<quint32> written;
 bool put(quint32 *data,int count){++attempts;if(full)return false;written.assign(data,data+count);return true;}
};
struct Sound {
 Buffer txBuffer;bool playing=true,playbackOk=true;
 bool isPlaying()const{return playing;}
 bool startPlayback(){events.push_back("play");playing=playbackOk;return playbackOk;}
} sound;
Sound *soundIOPtr=&sound;
class synthesizer {
public:
 double toneBuffer[TONEBUFLEN]={};unsigned pttToneCounter=0;
 bool writePcm(const double *,int,const std::atomic_bool &);
};
class txFunctions {
public:enum etxState{TXSENDKGSTV,TXSSTVIMAGE};
 void startTX(etxState){events.push_back("TX");}
};
struct Rig {void activatePTT(bool on){events.push_back(on?"PTT on":"PTT off");}} rig;
Rig *rigControllerPtr=&rig;
struct Controller {bool ready=true;bool prepared()const{return ready;}void completeTransmit(bool ok){assert(!ok);events.push_back("KG failed");}} controller;
Controller *kgstvControllerPtr=&controller;
constexpr int TRXKGSTV=2;int transmissionModeIndex=TRXKGSTV;
struct Widget {
 txFunctions functions;txFunctions *functionsPtr(){return &functions;}
 void slotStop(){events.push_back("stop");}
} widget;
Widget *txWidgetPtr=&widget;
class dispatcher {
public:bool radioTransition=false;void idleAll(){events.push_back("idle");}
 void startTX(txFunctions::etxState);
};
'''
checks = r'''
int main(){
 dispatcher dispatch;
 dispatch.radioTransition=true;dispatch.startTX(txFunctions::TXSENDKGSTV);assert(events.empty());dispatch.radioTransition=false;
 controller.ready=false;dispatch.startTX(txFunctions::TXSENDKGSTV);
 assert((events==std::vector<std::string>{"idle","stop"}));events.clear();controller.ready=true;

 sound.playbackOk=false;dispatch.startTX(txFunctions::TXSENDKGSTV);
 assert((events==std::vector<std::string>{"idle","play","PTT off","KG failed","stop"}));
 events.clear();dispatch.startTX(txFunctions::TXSSTVIMAGE);
 assert((events==std::vector<std::string>{"idle","play","PTT off","stop"}));
 events.clear();sound.playbackOk=true;dispatch.startTX(txFunctions::TXSENDKGSTV);
 assert((events==std::vector<std::string>{"idle","play","PTT on","TX"}));
 synthesizer synth;std::atomic_bool canceled(false);double samples[2]={0.7,-0.7};
 assert(synth.writePcm(samples,2,canceled));assert(sound.txBuffer.written.size()==2);
 assert(sound.txBuffer.written[0]==quint16(8000));assert(sound.txBuffer.written[1]==quint16(qint16(-8000)));
 swapChannel=true;assert(synth.writePcm(samples,2,canceled));
 assert(sound.txBuffer.written[1]==(quint32(quint16(qint16(-8000)))<<16));
 swapChannel=false;pttToneOtherChannel=true;std::fill(synth.toneBuffer,synth.toneBuffer+TONEBUFLEN,1234.0);
 assert(synth.writePcm(samples,2,canceled));assert((sound.txBuffer.written[1]>>16)==1234);
 swapChannel=true;assert(synth.writePcm(samples,2,canceled));assert((sound.txBuffer.written[1]&65535)==1234);
 swapChannel=false;pttToneOtherChannel=false;
 sound.txBuffer.written.clear();canceled.store(true);assert(!synth.writePcm(samples,2,canceled));assert(sound.txBuffer.written.empty());
 canceled.store(false);sound.playing=false;assert(!synth.writePcm(samples,2,canceled));
 sound.playing=true;sound.txBuffer.full=true;timeoutNow=true;assert(!synth.writePcm(samples,2,canceled));
 timeoutNow=false;cancelOnWait=&canceled;assert(!synth.writePcm(samples,2,canceled));assert(canceled.load());
 cancelOnWait=nullptr;canceled.store(false);sound.txBuffer.full=false;
 assert(!synth.writePcm(nullptr,2,canceled));assert(!synth.writePcm(samples,TXSTRIPE+1,canceled));
 assert(!synth.writePcm(samples,0,canceled));
 double nan=std::numeric_limits<double>::quiet_NaN();assert(!synth.writePcm(&nan,1,canceled));
 double clipped=1e100;assert(synth.writePcm(&clipped,1,canceled));assert(sound.txBuffer.written[0]<=32767);
}
'''
source = prefix + '\n' + method('src/dispatch/dispatcher.cpp', 'void dispatcher::startTX(')
source += '\n' + method('src/dsp/synthes.cpp', 'bool synthesizer::writePcm(') + '\n' + checks
with tempfile.TemporaryDirectory(prefix='qsstv-radio-paths-') as temporary:
    path = Path(temporary)
    (path / 'test.cpp').write_text(source)
    subprocess.run(['g++', '-std=c++11', '-Wall', '-Wextra', str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True, timeout=10)
print('PASS: production PTT start-failure release, successful start, KG-STV PCM cancellation/backpressure/timeout, stereo/PTT-tone packing and invalid sample bounds. Device-free doubles; physical CAT/PTT remains untested.')
