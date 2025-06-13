#include "SoundManager.h"
#include <QDebug>
#include <QMediaPlaylist>
#include <QRandomGenerator>

SoundManager& SoundManager::instance()
{
    static SoundManager instance;
    return instance;
}

SoundManager::SoundManager(QObject* parent)
    : QObject(parent)
    , m_BGMPlayer(nullptr)
    , m_volume(50) // 默认音量50%
{
    initializeSounds();
}

SoundManager::~SoundManager()
{
    stopBGM();
    qDeleteAll(m_soundEffects);
    delete m_BGMPlayer;
}

void SoundManager::initializeSounds()
{
    // 初始化BGM播放器
    m_BGMPlayer = new QMediaPlayer(this);
    
    // 初始化BGM列表
    m_bgmList.clear();
    m_bgmList << "qrc:/mus/res/BGM_1.wav"
              << "qrc:/mus/res/BGM_2.wav"
              << "qrc:/mus/res/BGM_3.wav";

    // 设置播放完成后的处理
    connect(m_BGMPlayer, &QMediaPlayer::stateChanged, this, [this](QMediaPlayer::State state) {
        if (state == QMediaPlayer::StoppedState) {
            // 当一首BGM播放完成后，随机播放下一首
            playRandomBGM();
        }
    });

    // 定义音效文件列表
    QMap<QString, QString> soundFiles;
    soundFiles["card_play"] = "qrc:/mus/res/card_play.mp3";
    soundFiles["card_deal"] = "qrc:/mus/res/card_deal.mp3";

    // 清理并重新加载音效
    qDeleteAll(m_soundEffects);
    m_soundEffects.clear();

    // 遍历音效文件列表，设置音频文件
    for (auto it = soundFiles.constBegin(); it != soundFiles.constEnd(); ++it) {
        // 创建 QMediaPlayer 对象
        QMediaPlayer* effectPlayer = new QMediaPlayer(this);
        effectPlayer->setMedia(QUrl(it.value()));
        m_soundEffects[it.key()] = effectPlayer;
    }

    updateVolume();
}

void SoundManager::playRandomBGM()
{
    if (m_bgmList.isEmpty()) {
        return;
    }

    // 生成随机索引
    int randomIndex = QRandomGenerator::global()->bounded(m_bgmList.size());
    
    // 设置并播放随机选择的BGM
    m_BGMPlayer->setMedia(QUrl(m_bgmList[randomIndex]));
    m_BGMPlayer->play();
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
    m_BGMPlayer->setVolume(qRound(volume * 100));

    for (QMediaPlayer* effectPlayer : m_soundEffects) {
        effectPlayer->setVolume(m_volume);
    }
}

void SoundManager::playBGM()
{
    if (m_BGMPlayer->state() != QMediaPlayer::PlayingState) {
        playRandomBGM();
    }
}

void SoundManager::stopBGM()
{
    m_BGMPlayer->stop();
}

void SoundManager::playCardPlaySound()
{
    if (m_soundEffects.contains("card_play")) {
        QMediaPlayer* player = m_soundEffects["card_play"];
        // 如果正在播放，先停止并回到开头，再播放，以实现快速重复触发
        if (player->state() == QMediaPlayer::PlayingState) {
            player->stop();
        }
        player->play();
    }
}

void SoundManager::playButtonClickSound()
{
    if (m_soundEffects.contains("card_deal")) {
        QMediaPlayer* player = m_soundEffects["card_deal"];
        if (player->state() == QMediaPlayer::PlayingState) {
            player->stop();
        }
        player->play();
    }
}