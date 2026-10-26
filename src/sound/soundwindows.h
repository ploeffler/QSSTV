#ifndef SOUNDWINDOWS_H
#define SOUNDWINDOWS_H

#include "soundbase.h"
#include <QAudioInput>
#include <QAudioOutput>
#include <QIODevice>

class soundWindows : public soundBase
{
public:
    soundWindows();
    ~soundWindows() override;
    bool init(int samplerate) override;
    int read(int &countAvailable) override;
    int write(uint numFrames) override;
protected:
    void flushCapture() override;
    void flushPlayback() override;
    void closeDevices() override;
    void waitPlaybackEnd() override;
private:
    QAudioInput *input;
    QAudioOutput *output;
    QIODevice *inputDevice;
    QIODevice *outputDevice;
};
#endif
