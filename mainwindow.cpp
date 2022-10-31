#include "mainwindow.hpp"

#include <fmt/chrono.h>
#include <fmt/core.h>

#include <QApplication>
#include <QChar>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFontMetricsF>
#include <QInputDialog>
#include <QMediaMetaData>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStringList>
#include <QUrl>
#include <chrono>
#include <ciso646>

#include "./ui_mainwindow.h"
#include "processwidget.hpp"

void MainWindow::update_current_pos_and_slider_(QTime time) {
    using std::chrono::duration_cast, std::chrono::milliseconds, std::chrono::seconds;
    milliseconds new_position(time.msecsSinceStartOfDay());
    this->update_video_position_slider_(duration_cast<seconds>(new_position));
    this->change_video_position_(new_position);
}
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    player_.setVideoOutput(ui->videoWidget);
    player_.setAudioOutput(&audio_output_);
    ui->rangeSlider_selected->SetOption(RangeSlider::DoubleHandles);
    ui->pushButton_save->setEnabled(false);
    connect(ui->pushButton_play_pause, &QPushButton::pressed, this, &MainWindow::play_pause_button_pressed_);
    connect(ui->horizontalSlider_current_pos, &QSlider::sliderMoved, [=](int position) {
        if (ui->horizontalSlider_current_pos->isSliderDown()) {
            std::chrono::seconds new_position(position);
            this->update_video_position_display_(new_position);
            this->change_video_position_(new_position);
        }
    });
    connect(ui->timeEdit_current_time, &QTimeEdit::timeChanged, this, &MainWindow::update_current_pos_and_slider_);
    connect(ui->timeEdit_start_time, &QTimeEdit::userTimeChanged, [=](QTime time) {
        auto new_position =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(time.msecsSinceStartOfDay()));
        this->update_start_time_slider_(new_position);
    });
    connect(ui->timeEdit_end_time, &QTimeEdit::userTimeChanged, [=](QTime time) {
        auto new_position =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(time.msecsSinceStartOfDay()));
        this->update_end_time_slider_(new_position);
    });
    connect(ui->rangeSlider_selected, &RangeSlider::lowerValueSliderMoved, [=](int position) {
        std::chrono::seconds new_position(position);
        this->update_start_time_display_(new_position);
    });
    connect(ui->rangeSlider_selected, &RangeSlider::upperValueSliderMoved, [=](int position) {
        std::chrono::seconds new_position(position);
        this->update_end_time_display_(new_position);
    });
    connect(ui->pushButton_set_to_end, &QPushButton::clicked, [=] {
        ui->timeEdit_end_time->setTime(QTime::fromMSecsSinceStartOfDay(this->player_.duration()));
        ui->rangeSlider_selected->setUpperValue(ui->rangeSlider_selected->GetMaximun());
    });
    connect(ui->pushButton_end_set_to_current, &QPushButton::clicked,
            [=] { ui->timeEdit_end_time->setTime(ui->timeEdit_current_time->time()); });
    connect(ui->pushButton_start_set_to_current, &QPushButton::clicked,
            [=] { ui->timeEdit_start_time->setTime(ui->timeEdit_current_time->time()); });
    connect(ui->actionopen, &QAction::triggered, this, &MainWindow::open_video_);
    connect(ui->actionclose, &QAction::triggered, this, &MainWindow::close_video_);
    connect(ui->pushButton_save, &QPushButton::pressed, this, &MainWindow::save_result_);
    connect(&player_, &QMediaPlayer::errorOccurred, this, &MainWindow::show_videoerror_);
    connect(&player_, &QMediaPlayer::durationChanged, this, &MainWindow::update_duration_);
    connect(&player_, &QMediaPlayer::positionChanged, [=](qint64 position) {
        if (ui->horizontalSlider_current_pos->isSliderDown()) {
            return;
        }
        using std::chrono::duration_cast, std::chrono::milliseconds, std::chrono::seconds;
        milliseconds new_position{position};
        this->update_video_position_display_(new_position);
        this->update_video_position_slider_(duration_cast<seconds>(new_position));
    });
    connect(ui->actionenable_tracking_of_current_time_slider, &QAction::triggered, this,
            &MainWindow::change_curent_time_slider_tracking_state_);
    connect(ui->actionsavefile_name_plugin, &QAction::triggered, this, &MainWindow::select_savefile_name_plugin_);

    QDir settings_dir(QApplication::applicationDirPath() + "/settings");
    if (QDir().mkpath(settings_dir.absolutePath())) {  // QDir::mkpath() returns true even when path already exists
        settings_ = new QSettings(settings_dir.filePath("settings.ini"), QSettings::IniFormat);
        if (settings_->contains("savefile_name_plugin") && settings_->value("savefile_name_plugin") != NO_PLUGIN) {
            savefile_name_plugin_ =
                savefile_name_plugins_dir_().absoluteFilePath(settings_->value("savefile_name_plugin").toString());
        }
    }
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::play_pause_button_pressed_() {
    if (ui->pushButton_play_pause->text() == tr("play")) {
        player_.play();
        ui->pushButton_play_pause->setText(tr("pause"));
    } else {
        player_.pause();
        ui->pushButton_play_pause->setText(tr("play"));
    }
}

void MainWindow::update_video_position_display_(std::chrono::milliseconds newposition) {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    milliseconds current_position{this->ui->timeEdit_current_time->time().msecsSinceStartOfDay()};
    if (newposition != current_position) {
        // disable update_current_pos_and_slider() for the setTime()
        disconnect(ui->timeEdit_current_time, &QTimeEdit::timeChanged, this,
                   &MainWindow::update_current_pos_and_slider_);
        connect(
            ui->timeEdit_current_time, &QTimeEdit::timeChanged, this,
            [=] {
                connect(this->ui->timeEdit_current_time, &QTimeEdit::timeChanged, this,
                        &MainWindow::update_current_pos_and_slider_);
            },
            static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::SingleShotConnection));
        ui->timeEdit_current_time->setTime(QTime::fromMSecsSinceStartOfDay(newposition.count()));
        qDebug() << fmt::format("@{} {}", __func__, newposition).c_str();
    }
}
void MainWindow::update_video_position_slider_(std::chrono::seconds newposition) {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    auto current_position = seconds(ui->horizontalSlider_current_pos->value());
    if (newposition != current_position) {
        ui->horizontalSlider_current_pos->setValue(newposition.count());
        qDebug() << fmt::format("@{} {}", __func__, newposition).c_str();
    }
}
void MainWindow::change_video_position_(std::chrono::milliseconds newposition) {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    milliseconds current_position(player_.position());
    if (newposition != current_position) {
        player_.setPosition(newposition.count());
        qDebug() << fmt::format("@{} {}", __func__, newposition).c_str();
    }
}

void MainWindow::update_start_time_display_(std::chrono::seconds time) {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    auto current_position =
        duration_cast<seconds>(milliseconds(this->ui->timeEdit_start_time->time().msecsSinceStartOfDay()));
    if (time != current_position) {
        ui->timeEdit_start_time->setTime(QTime::fromMSecsSinceStartOfDay(duration_cast<milliseconds>(time).count()));
        qDebug() << fmt::format("@{} {}", __func__, time).c_str();
    }
}
void MainWindow::update_start_time_slider_(std::chrono::seconds time) {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    auto current_position = seconds(ui->rangeSlider_selected->GetLowerValue());
    if (time != current_position) {
        ui->rangeSlider_selected->setLowerValue(time.count());
        qDebug() << fmt::format("@{} {}", __func__, time).c_str();
    }
}

void MainWindow::update_end_time_display_(std::chrono::seconds time) {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    auto current_position =
        duration_cast<seconds>(milliseconds(this->ui->timeEdit_end_time->time().msecsSinceStartOfDay()));
    if (time != current_position) {
        ui->timeEdit_end_time->setTime(QTime::fromMSecsSinceStartOfDay(duration_cast<milliseconds>(time).count()));
        qDebug() << fmt::format("@{} {}", __func__, time).c_str();
    }
}
void MainWindow::update_end_time_slider_(std::chrono::seconds time) {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    auto current_position = seconds(ui->rangeSlider_selected->GetUpperValue());
    if (time != current_position) {
        ui->rangeSlider_selected->setUpperValue(time.count());
        qDebug() << fmt::format("@{} {}", __func__, time).c_str();
    }
}

void MainWindow::open_video_() {
    auto videodirs = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);
    qDebug() << videodirs;
    qDebug() << QUrl(videodirs.isEmpty() ? QUrl() : QUrl(QDir(videodirs[0]).path()));
    auto fileurl = QFileDialog::getOpenFileUrl(this, tr("open video file"),
                                               videodirs.isEmpty() ? QUrl() : QUrl::fromLocalFile(videodirs[0]));
    if (fileurl != QUrl()) {
        player_.setSource(fileurl);
        ui->pushButton_save->setEnabled(true);
        ui->pushButton_play_pause->setText(tr("play"));
    } else {
        QMessageBox::warning(nullptr, tr("warning"), tr("no file was selected."));
    }
}
void MainWindow::close_video_() {
    player_.setSource(QUrl());
    ui->pushButton_save->setEnabled(false);
}
void MainWindow::update_duration_(int newduration) {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    using namespace std::literals::chrono_literals;
    seconds length = duration_cast<seconds>(milliseconds(newduration));
    if (length != 0s) {
        ui->horizontalSlider_current_pos->setMaximum(length.count());
        ui->rangeSlider_selected->setMaximum(length.count());
        ui->rangeSlider_selected->setLowerValue(0);
        ui->rangeSlider_selected->setUpperValue(length.count());
        ui->timeEdit_start_time->setTime(QTime::fromMSecsSinceStartOfDay(0));
        ui->timeEdit_end_time->setTime(QTime::fromMSecsSinceStartOfDay(newduration));
    }
}

void MainWindow::show_videoerror_(QMediaPlayer::Error, const QString &error_string) {
    QMessageBox::critical(this, tr("Error!"), error_string);
    player_.setSource(QUrl());
    ui->pushButton_save->setEnabled(false);
}

void MainWindow::save_result_() { start_saving_(); }
void MainWindow::start_saving_() {
    auto source_filename = player_.source().fileName();
    process_ = new ProcessWidget(this, Qt::Window);
    process_->setWindowModality(Qt::ApplicationModal);
    process_->setAttribute(Qt::WA_DeleteOnClose, true);
    process_->show();
    if (savefile_name_plugin_.has_value()) {
        using std::chrono::duration;
        using std::chrono::duration_cast;
        using std::chrono::milliseconds;
        process_->start(PYTHON,
                        // clang-format off
            {savefile_name_plugin_.value(),
            "--filename", source_filename,
            "--back-offset",
            QString::number(
                duration_cast<duration<double>>(
                    milliseconds(player_.duration() - ui->timeEdit_end_time->time().msecsSinceStartOfDay()))
                    .count())
            },
                        // clang-format on
                        false);
        connect(
            process_, &ProcessWidget::finished, this,
            [=](bool is_success) {
                if (is_success) {
                    this->confirm_save_filename_(this->process_->get_stdout().remove("\n"));
                }
            },
            static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::SingleShotConnection));
    } else {
        this->confirm_save_filename_(source_filename);
    }
}
void MainWindow::confirm_save_filename_(QString default_save_filename) {
    bool confirmed = false;
    QString save_filename;
    QString save_filepath;
    bool overwrite = false;
    do {
        save_filename = QInputDialog::getText(this, tr("savefile name"), tr("enter file name of result"),
                                              QLineEdit::Normal, default_save_filename, &confirmed);
        save_filepath = QUrl(player_.source().toString(QUrl::RemoveFilename) + save_filename).toLocalFile();
        if (not confirmed) {
            return;
        }
        if (save_filename.isEmpty()) {
            auto button = QMessageBox::warning(this, tr("empty filename"), tr("filename cannot be empty"),
                                               QMessageBox::Retry | QMessageBox::Abort, QMessageBox::Retry);
            if (button == QMessageBox::Abort) {
                return;
            }
        } else if (QFile::exists(save_filepath)) {
            auto button =
                QMessageBox::warning(this, tr("overwrite source"),
                                     tr("provided filename already exists. Are you sure you want to OVERWRITE it?"),
                                     QMessageBox::Retry | QMessageBox::Abort | QMessageBox::Yes, QMessageBox::Retry);
            switch (button) {
                case QMessageBox::Retry:
                    save_filename = "";
                    break;
                case QMessageBox::Abort:
                    return;
                case QMessageBox::Yes:
                    overwrite = true;
                    break;
                default:
                    Q_UNREACHABLE();
            }
        } else {
            break;
        }
    } while (save_filename.isEmpty());
    if (overwrite) {
        // TODO: add things needed for overwriting file (e.g. delete existing file)
        QMessageBox::critical(nullptr, "not implemented error", "file overwriting is not implemented");
        return;
    }
    cut_video_(save_filepath);
}
namespace impl_ {
int decode_ffmpeg(QStringView, QStringView new_stderr) {
    QRegularExpression time_pattern(R"(time=(?<hours>\d\d):(?<minutes>\d\d):(?<seconds>\d\d).(?<centiseconds>\d\d))");
    auto match = time_pattern.match(new_stderr);
    if (not match.hasMatch()) {
        return -1;
    }
    using std::chrono::duration_cast;
    using std::chrono::hours;
    using std::chrono::minutes;
    using std::chrono::seconds;
    using centiseconds = std::chrono::duration<int, std::centi>;
    using std::chrono::milliseconds;
    return duration_cast<milliseconds>(
               hours(match.captured("hours").toInt()) + minutes(match.captured("minutes").toInt()) +
               seconds(match.captured("seconds").toInt()) + centiseconds(match.captured("centiseconds").toInt()))
        .count();
}
}  // namespace impl_
void MainWindow::cut_video_(QString save_filepath) {
    auto range_start = ui->timeEdit_start_time->time();
    auto range_end = ui->timeEdit_end_time->time();
    auto source_filepath = player_.source().toLocalFile();
    QStringList arguments;
    // clang-format off
    arguments << "-ss" << range_start.toString("hh:mm:ss.zzz");
    if (range_end.msecsSinceStartOfDay() != player_.duration()) {
        arguments << "-to" << range_end.toString("hh:mm:ss.zzz");
    }
    arguments << "-i"  << source_filepath
              << "-c"  << "copy"
              << save_filepath;
    // clang-format on
    process_->start("ffmpeg", arguments, true,
                    {0, (range_end.msecsSinceStartOfDay() - range_start.msecsSinceStartOfDay()), impl_::decode_ffmpeg});
}

void MainWindow::change_curent_time_slider_tracking_state_(bool state) {
    ui->horizontalSlider_current_pos->setTracking(state);
}

int MainWindow::savefile_name_plugin_index_() {
    int result = 0;
    if (settings_->contains("savefile_name_plugin")) {
        int raw_idx = savefile_name_plugins_().indexOf(settings_->value("savefile_name_plugin"));
        result = qMax(raw_idx, 0);
    }

    return result;
}
QDir MainWindow::savefile_name_plugins_dir_() {
    return QDir(QApplication::applicationDirPath() + "/plugins/savefile_name");
}
QStringList MainWindow::search_savefile_name_plugins_() {
    return savefile_name_plugins_dir_().entryList({"*.py"}, QDir::Files);
}

QStringList MainWindow::savefile_name_plugins_() { return QStringList{NO_PLUGIN} + search_savefile_name_plugins_(); }

void MainWindow::select_savefile_name_plugin_() {
    QString plugin = NO_PLUGIN;
    bool confirmed;
    plugin = QInputDialog::getItem(this, tr("select plugin"), tr("Select savefile name plugin."),
                                   savefile_name_plugins_(), savefile_name_plugin_index_(), false, &confirmed);
    if (confirmed && settings_ != nullptr) {
        settings_->setValue("savefile_name_plugin", plugin);
    }
    if ((not confirmed) || plugin == NO_PLUGIN) {
        savefile_name_plugin_ = std::nullopt;
    } else {
        savefile_name_plugin_ = savefile_name_plugins_dir_().absoluteFilePath(plugin);
    }
}
