/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "viewer.h"

#include <QDateTime>
#include <QLabel>
#include <QResizeEvent>
#include <QtMath>
#include <QVBoxLayout>

#include "viewersizer.h"

ViewerWidget::ViewerWidget(QWidget *parent) :
  QWidget(parent)
{
  // Set up main layout
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setMargin(0);

  // Create main OpenGL-based view
  ViewerSizer* sizer = new ViewerSizer(this);
  layout->addWidget(sizer);

  gl_widget_ = new ViewerGLWidget(this);
  sizer->SetWidget(gl_widget_);

  // FIXME: Hardcoded values
  sizer->SetChildSize(1920, 1080);

  // Create time ruler
  ruler_ = new TimeRuler(false, this);
  layout->addWidget(ruler_);
  connect(ruler_, SIGNAL(TimeChanged(int64_t)), this, SLOT(RulerTimeChange(int64_t)));

  // Create scrollbar
  scrollbar_ = new QScrollBar(Qt::Horizontal, this);
  layout->addWidget(scrollbar_);
  connect(scrollbar_, SIGNAL(valueChanged(int)), ruler_, SLOT(SetScroll(int)));
  scrollbar_->setPageStep(ruler_->width());

  // Create lower controls
  controls_ = new PlaybackControls(this);
  controls_->SetTimecodeEnabled(true);
  controls_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  connect(controls_, SIGNAL(PlayClicked()), this, SLOT(Play()));
  connect(controls_, SIGNAL(PauseClicked()), this, SLOT(Pause()));
  connect(controls_, SIGNAL(PrevFrameClicked()), this, SLOT(PrevFrame()));
  connect(controls_, SIGNAL(NextFrameClicked()), this, SLOT(NextFrame()));
  connect(controls_, SIGNAL(BeginClicked()), this, SLOT(GoToStart()));
  connect(controls_, SIGNAL(EndClicked()), this, SLOT(GoToEnd()));
  layout->addWidget(controls_);

  // Connect timer
  connect(&playback_timer_, SIGNAL(timeout()), this, SLOT(PlaybackTimerUpdate()));

  // FIXME: Magic number
  ruler_->SetScale(48.0);
}

void ViewerWidget::SetTimebase(const rational &r)
{
  time_base_ = r;
  time_base_dbl_ = r.toDouble();

  ruler_->SetTimebase(r);
  controls_->SetTimebase(r);

  playback_timer_.setInterval(qFloor(r.toDouble()));
}

const double &ViewerWidget::scale()
{
  return ruler_->scale();
}

rational ViewerWidget::GetTime()
{
  return rational(ruler_->GetTime()) * time_base_;
}

void ViewerWidget::SetScale(const double &scale_)
{
  ruler_->SetScale(scale_);
}

void ViewerWidget::SetTime(const int64_t &time)
{
  ruler_->SetTime(time);
  UpdateTimeInternal(time);
}

void ViewerWidget::TogglePlayPause()
{
  if (IsPlaying()) {
    Pause();
  } else {
    Play();
  }
}

bool ViewerWidget::IsPlaying()
{
  return playback_timer_.isActive();
}

void ViewerWidget::SetTexture(GLuint tex)
{
  gl_widget_->SetTexture(tex);
}

void ViewerWidget::UpdateTimeInternal(int64_t i)
{
  rational time_set = rational(i) * time_base_;

  controls_->SetTime(i);

  emit TimeChanged(time_set);
}

void ViewerWidget::RulerTimeChange(int64_t i)
{
  Pause();

  UpdateTimeInternal(i);
}

void ViewerWidget::Play()
{
  if (time_base_.isNull()) {
    qWarning() << "ViewerWidget can't play with an invalid timebase";
    return;
  }

  start_msec_ = QDateTime::currentMSecsSinceEpoch();
  start_timestamp_ = ruler_->GetTime();

  playback_timer_.start();

  controls_->ShowPauseButton();
}

void ViewerWidget::Pause()
{
  playback_timer_.stop();

  controls_->ShowPlayButton();
}

void ViewerWidget::GoToStart()
{
  Pause();

  SetTime(0);
}

void ViewerWidget::PrevFrame()
{
  Pause();

  SetTime(qMax(static_cast<int64_t>(0), ruler_->GetTime() - 1));
}

void ViewerWidget::NextFrame()
{
  Pause();

  SetTime(ruler_->GetTime() + 1);
}

void ViewerWidget::GoToEnd()
{
  Pause();

  qWarning() << "No end frame support yet";
}

void ViewerWidget::PlaybackTimerUpdate()
{
  int64_t real_time = QDateTime::currentMSecsSinceEpoch() - start_msec_;

  int64_t frames_since_start = qRound(static_cast<double>(real_time) / (time_base_dbl_ * 1000));

  SetTime(start_timestamp_ + frames_since_start);
}

void ViewerWidget::resizeEvent(QResizeEvent *event)
{
  // Set scrollbar page step to the width
  scrollbar_->setPageStep(event->size().width());
}
