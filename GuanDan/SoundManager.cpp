#include "SoundManager.h"
#include <QDebug>

SoundManager& SoundManager::instance()
{
    static SoundManager instance;
    return instance;
}

SoundManager::SoundManager(QObject* parent)
    : QObject(parent)
    , m_bgmPlayer(nullptr)
    , m_audioOutput(nullptr)
    , m_volume(50) // 默认音量50%
{
    initializeSounds();
}

SoundManager::~SoundManager()
{
    stopBGM();
    qDeleteAll(m_soundEffects);
    delete m_bgmPlayer;
    delete m_audioOutput;
}

void SoundManager::initializeSounds()
{
    // 初始化BGM播放器
    m_bgmPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_bgmPlayer->setAudioOutput(m_audioOutput);
    m_bgmPlayer->setSource(QUrl("qrc:/sounds/res/sounds/bgm.mp3"));
    m_bgmPlayer->setLoops(QMediaPlayer::Infinite);

    // 初始化音效
    QStringList soundFiles = {
        "card_deal", "card_play", "win", "lose", "button_click"
    };

    for (const QString& sound : soundFiles) {
        QSoundEffect* effect = new QSoundEffect(this);
        effect->setSource(QUrl(QString("qrc:/sounds/res/sounds/%1.wav").arg(sound)));
        effect->setVolume(m_volume / 100.0f);
        m_soundEffects[sound] = effect;
    }

    updateVolume();
}

void SoundManager::setVolume(int volume)
{
    m_volume = qBound(0, volume, 100);
    updateVolume();
}

int SoundManager::getVolume() const
{
    return m_volume;
}

void SoundManager::updateVolume()
{
    float volume = m_volume / 100.0f;
    m_audioOutput->setVolume(volume);
    
    for (QSoundEffect* effect : m_soundEffects) {
        effect->setVolume(volume);
    }
}

void SoundManager::playBGM()
{
    if (m_bgmPlayer->playbackState() != QMediaPlayer::PlayingState) {
        m_bgmPlayer->play();
    }
}

void SoundManager::stopBGM()
{
    m_bgmPlayer->stop();
}

void SoundManager::playCardDealSound()
{
    if (m_soundEffects.contains("card_deal")) {
        m_soundEffects["card_deal"]->play();
    }
}

void SoundManager::playCardPlaySound()
{
    if (m_soundEffects.contains("card_play")) {
        m_soundEffects["card_play"]->play();
    }
}

void SoundManager::playWinSound()
{
    if (m_soundEffects.contains("win")) {
        m_soundEffects["win"]->play();
    }
}

void SoundManager::playLoseSound()
{
    if (m_soundEffects.contains("lose")) {
        m_soundEffects["lose"]->play();
    }
}

void SoundManager::playButtonClickSound()
{
    if (m_soundEffects.contains("button_click")) {
        m_soundEffects["button_click"]->play();
    }
}
