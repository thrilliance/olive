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

#include "decoder.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>

#include "decoder/ffmpeg/ffmpegdecoder.h"
#include "decoder/oiio/oiiodecoder.h"

Decoder::Decoder() :
  open_(false),
  stream_(nullptr)
{
}

Decoder::Decoder(Stream *fs) :
  open_(false),
  stream_(fs)
{
}

Decoder::~Decoder()
{
}

StreamPtr Decoder::stream()
{
  return stream_;
}

void Decoder::set_stream(StreamPtr fs)
{
  Close();

  stream_ = fs;
}

/*
 * DECODER STATIC PUBLIC MEMBERS
 */

QVector<DecoderPtr> ReceiveListOfAllDecoders() {
  QVector<DecoderPtr> decoders;

  // The order in which these decoders are added is their priority when probing. Hence FFmpeg should usually be last,
  // since it supports so many formats and we presumably want to override those formats with a more specific decoder.

  decoders.append(std::make_shared<OIIODecoder>());
  decoders.append(std::make_shared<FFmpegDecoder>());

  return decoders;
}

bool Decoder::ProbeMedia(Footage *f)
{
  // Check for a valid filename
  if (f->filename().isEmpty()) {
    qWarning() << QCoreApplication::translate("ProbeMedia", "Tried to probe media with an empty filename");
    return false;
  }

  // Check file exists
  if (!QFileInfo::exists(f->filename())) {
    qWarning() << QCoreApplication::translate("ProbeMedia", "Tried to probe file that doesn't exist");
    return false;
  }

  // Reset Footage state for probing
  f->Clear();

  // Create list to iterate through
  QVector<DecoderPtr> decoder_list = ReceiveListOfAllDecoders();

  // Pass Footage through each Decoder's probe function
  for (int i=0;i<decoder_list.size();i++) {

    DecoderPtr decoder = decoder_list.at(i);

    if (decoder->Probe(f)) {

      // We found a Decoder, so we can set this media as valid
      f->set_status(Footage::kReady);

      // Attach the successful Decoder to this Footage object
      f->set_decoder(decoder->id());

      // FIXME: Cache the results so we don't have to probe if this media is added a second time

      return true;
    }
  }

  // We aren't able to use this Footage
  f->set_status(Footage::kInvalid);
  f->set_decoder(QString());

  return false;
}

DecoderPtr Decoder::CreateFromID(const QString &id)
{
  if (id.isEmpty()) {
    return nullptr;
  }

  // Create list to iterate through
  QVector<DecoderPtr> decoder_list = ReceiveListOfAllDecoders();

  foreach (DecoderPtr d, decoder_list) {
    if (d->id() == id) {
      return d;
    }
  }

  return nullptr;
}
