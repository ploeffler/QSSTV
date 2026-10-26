#include "soundwindows.h"
#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QThread>
#include <cstring>

soundWindows::soundWindows() : input(nullptr), output(nullptr),
    inputDevice(nullptr), outputDevice(nullptr) {}

soundWindows::~soundWindows() { closeDevices(); }

bool soundWindows::init(int samplerate)
{
    closeDevices();
    sampleRate = samplerate;
    soundDriverOK = false;
    QAudioFormat inputFormat;
    inputFormat.setSampleRate(sampleRate);
    inputFormat.setChannelCount(1);
    inputFormat.setSampleSize(16);
    inputFormat.setCodec("audio/pcm");
    inputFormat.setByteOrder(QAudioFormat::LittleEndian);
    inputFormat.setSampleType(QAudioFormat::SignedInt);
    QAudioFormat outputFormat = inputFormat;
    outputFormat.setChannelCount(2);

    const QAudioDeviceInfo inDevice = QAudioDeviceInfo::defaultInputDevice();
    const QAudioDeviceInfo outDevice = QAudioDeviceInfo::defaultOutputDevice();
    if (inDevice.isNull() || outDevice.isNull() ||
        !inDevice.isFormatSupported(inputFormat) ||
        !outDevice.isFormatSupported(outputFormat)) {
        errorHandler("Windows audio error", "Default audio devices must support 16-bit PCM at the configured sample rate (mono capture, stereo playback).");
        return false;
    }
    input = new QAudioInput(inDevice, inputFormat);
    output = new QAudioOutput(outDevice, outputFormat);
    input->setBufferSize(DOWNSAMPLESIZE * sizeof(qint16) * 8);
    output->setBufferSize(DOWNSAMPLESIZE * sizeof(quint32) * 8);
    inputDevice = input->start();
    outputDevice = output->start();
    if (!inputDevice || !outputDevice) {
        errorHandler("Windows audio error", "Unable to start audio input/output.");
        closeDevices();
        return false;
    }
    isStereo = false;
    soundDriverOK = true;
    return true;
}

int soundWindows::read(int &countAvailable)
{
    if (!soundDriverOK || !inputDevice) return 0;
    countAvailable = static_cast<int>(input->bytesReady() / sizeof(qint16));
    if (countAvailable < DOWNSAMPLESIZE) return 0;
    const qint64 bytes = DOWNSAMPLESIZE * sizeof(qint16);
    return inputDevice->read(reinterpret_cast<char *>(tempRXBuffer), bytes) == bytes
        ? DOWNSAMPLESIZE : 0;
}

int soundWindows::write(uint numFrames)
{
    if (!soundDriverOK || !outputDevice) return 0;
    const qint64 bytes = static_cast<qint64>(numFrames) * sizeof(quint32);
    if (output->bytesFree() < bytes) return 0;
    return outputDevice->write(reinterpret_cast<const char *>(tempTXBuffer), bytes) == bytes
        ? static_cast<int>(numFrames) : 0;
}

void soundWindows::flushCapture()
{
    if (!inputDevice) return;
    char discard[4096];
    while (input->bytesReady() > 0) {
        if (inputDevice->read(discard, sizeof(discard)) <= 0) break;
    }
}
void soundWindows::flushPlayback() {}
void soundWindows::waitPlaybackEnd()
{
    if (!output) return;
    while (output->state() == QAudio::ActiveState && output->bufferSize() > output->bytesFree())
        QThread::msleep(10);
}
void soundWindows::closeDevices()
{
    soundDriverOK = false;
    if (input) { input->stop(); delete input; input = nullptr; }
    if (output) { output->stop(); delete output; output = nullptr; }
    inputDevice = nullptr;
    outputDevice = nullptr;
}
