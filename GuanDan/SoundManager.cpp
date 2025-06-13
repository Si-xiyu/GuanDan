#include "SoundManager.h"
#include <QDebug>
#include <QMediaPlaylist>

SoundManager& SoundManager::instance()
{
    static SoundManager instance;
    return instance;
}

SoundManager::SoundManager(QObject* parent)
    : QObject(parent)
    , m_Player(nullptr)
    , m_volume(50) // 默认音量50%
{
    initializeSounds();
}

SoundManager::~SoundManager()
{
    stopBGM();
    qDeleteAll(m_soundEffects);
    delete m_Player;
}

void SoundManager::initializeSounds()
{
    // 初始化BGM播放器
    m_Player = new QMediaPlayer(this);
    QMediaPlaylist* playlist = new QMediaPlaylist(this);

    playlist->addMedia(QUrl("qrc:/mus/res/BGM_1.wav"));
    playlist->addMedia(QUrl("qrc:/mus/res/BGM_2.wav"));
    playlist->addMedia(QUrl("qrc:/mus/res/BGM_3.wav"));

    playlist->setPlaybackMode(QMediaPlaylist::Loop);
    m_Player->setPlaylist(playlist);

    // 定义音效文件列表
    QMap<QString, QString> soundFiles;
    soundFiles["card_play"] = "qrc:/mus/res/card_play.MP3";
    // 你的代码中，点击按钮播放的是"card_deal"音效，所以我们加载这个文件
    soundFiles["card_deal"] = "qrc:/mus/res/card_deal.MP3";

    // 清理并重新加载音效
    qDeleteAll(m_soundEffects);
    m_soundEffects.clear();

    // 使用正确的循环方式来遍历QMap
    for (auto it = soundFiles.constBegin(); it != soundFiles.constEnd(); ++it) {
        QSoundEffect* effect = new QSoundEffect(this);
        // 使用 it.value() 获取文件路径
        effect->setSource(QUrl(it.value()));
        effect->setVolume(m_volume / 100.0f);
        // 使用 it.key() 获取别名作为键
        m_soundEffects[it.key()] = effect;
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
    m_Player->setVolume(qRound(volume * 100));

    for (QSoundEffect* effect : m_soundEffects) {
        effect->setVolume(volume);
    }
}

void SoundManager::playBGM()
{
    if (m_Player->state() != QMediaPlayer::PlayingState) {
        m_Player->play();
    }
}

void SoundManager::stopBGM()
{
    m_Player->stop();
}

void SoundManager::playCardPlaySound()
{
    if (m_soundEffects.contains("card_play")) {
        m_soundEffects["card_play"]->play();
    }
}

void SoundManager::playButtonClickSound()
{
    // 这个函数会播放 "card_deal" 音效，因为这是你在 soundFiles 中定义的
    if (m_soundEffects.contains("card_deal")) {
        m_soundEffects["card_deal"]->play();
    }
}