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

#ifndef DECODER_H
#define DECODER_H

#include <QObject>
#include <stdint.h>

#include "common/rational.h"
#include "project/item/footage/footage.h"
#include "decoder/frame.h"

class Decoder;
using DecoderPtr = std::shared_ptr<Decoder>;

/**
 * @brief A decoder's is the main class for bringing external media into Olive
 *
 * Its responsibilities are to serve as
 * abstraction from codecs/decoders and provide complete frames. These frames can be video or audio data and are
 * provided as Frame objects in shared pointers to alleviate the responsibility of memory handling.
 *
 * The main function in a decoder is Retrieve() which should return complete image/audio data. A decoder should
 * alleviate all the complexities of codec compression from the rest of the application (i.e. a decoder should never
 * return a partial frame or require other parts of the system to interface directly with the codec). Often this will
 * necessitate pre-emptively caching, indexing, or even fully transcoding media before using it which can be implemented
 * through the Analyze() function.
 *
 * A decoder does NOT perform any pixel/sample format conversion. Frames should pass through the PixelFormatConverter
 * (olive::pix_fmt_conv) to be utilized in the rest of the rendering pipeline.
 */
class Decoder : public QObject
{
  Q_OBJECT
public:
  Decoder();

  Decoder(Stream* fs);

  // Necessary for subclassing, it's empty
  virtual ~Decoder();

  /**
   * @brief Deleted copy constructor
   */
  Decoder(const Decoder& other) = delete;

  /**
   * @brief Deleted move constructor
   */
  Decoder(Decoder&& other) = delete;

  /**
   * @brief Deleted copy assignment
   */
  Decoder& operator=(const Decoder& other) = delete;

  /**
   * @brief Deleted move assignment
   */
  Decoder& operator=(Decoder&& other) = delete;

  virtual QString id() = 0;

  StreamPtr stream();
  void set_stream(StreamPtr fs);

  /**
   * @brief Probe a footage file and dump metadata about it
   *
   * When a Footage file is imported, we'll need to know whether Olive is equipped with a decoder for utilizing it
   * and metadata should be retrieved about it if so. For this purpose, the Footage object is passed through all
   * Probe() functions of available deocders until one returns TRUE. A FALSE return means the Decoder was unable to
   * parse this file and the next should be tried.
   *
   * Probe() differs from Open() since it focuses on a file as a whole rather than one particular stream. Probe()
   * should be able to be run directly without calling Open() or Close() and should free its memory before returning.
   *
   * Probe() will never be called on an object that is also used for decoding. In other words, it will never be called
   * alongside Open() or Close() externally, so Probe() can use variables that would otherwise be used for decoding
   * without conflict.
   *
   * @param f
   *
   * A Footage object to probe. The Footage object will have a valid filename and will be empty prior to being sent
   * to this function (i.e. Footage::Clear() will not have to be called).
   *
   * @return
   *
   * TRUE if the Decoder was able to decode this file. FALSE if not. This function should have filled the Footage
   * object with metadata if it returns TRUE. Otherwise, the Footage object should be untouched.
   */
  virtual bool Probe(Footage* f) = 0;

  /**
   * @brief Open media/allocate memory
   *
   * Any file handles or memory allocation that needs to be done before this instance of a Decoder can return data
   * should be done here.
   *
   * @return
   *
   * TRUE if successful and ready to return data, FALSE if failed to open and unable to retrieve data. If the function
   * fails, any memory allocated should be free'd before returning FALSE, possibly by calling Close().
   */
  virtual bool Open() = 0;

  /**
   * @brief Retrieve frame/data
   *
   * The main function for retrieving data from the Decoder. This function should always provide complete frame data
   * (i.e. no partial frames or missing samples) at the timecode provided. The Decoder should perform any steps
   * required to retrieve a complete frame separate from the rest of the program, using any form of caching/indexing
   * to keep this as performant as possible.
   *
   * It's acceptable for this function to check whether the Decoder is open, and call Open() if not. If Open() returns
   * false, this function should return nullptr.
   *
   * @param timecode
   *
   * The timecode (a rational in seconds) to retrieve the data at.
   *
   * @param length
   *
   * Audio only - ignored for video decoders. The total length of audio data to retrieve (a rational in seconds).
   *
   * @return
   *
   * A FramePtr of valid data at this timecode (of the requested length if this is audio media), or nullptr if there
   * was nothing to retrieve at the provided timecode or the media could not be opened.
   */
  virtual FramePtr Retrieve(const rational& timecode, const rational& length = 0) = 0;

  /**
   * @brief Close media/deallocate memory
   *
   * Any file handles or memory allocations opened in Open() should be cleaned up here.
   *
   * As the main memory freeing function, it's good practice to call this in Open() if there's an error that prevents
   * correct function before Open() returns. As such, Close() should be prepared for not all memory/file handles to
   * have been opened successfully.
   */
  virtual void Close() = 0;

  /**
   * @brief Get a media file's internal timestamp
   *
   * Used to determine which frame will be served at a given time, useful for caching.
   */
  virtual int64_t GetTimestampFromTime(const rational& time) = 0;

  /**
   * @brief Try to probe a Footage file by passing it through all available Decoders
   *
   * This is a helper function designed to abstract the process of communicating with several Decoders from the rest of
   * the application. This function will take a Footage file and manually pass it through the available Decoders' Probe()
   * functions until one indicates that it can decode this file. That Decoder will then dump information about the file
   * into the Footage object for use throughout the program.
   *
   * Probing may be a lengthy process and it's recommended to run this in a separate thread.
   *
   * @param f
   *
   * A Footage object with a valid filename. If the Footage does not have a valid filename (e.g. is empty or file doesn't
   * exist), this function will return FALSE.
   *
   * @return
   *
   * TRUE if a Decoder was successfully able to parse and probe this file. FALSE if not.
   */
  static bool ProbeMedia(Footage* f);

  /**
   * @brief Create a Decoder instance using a Decoder ID
   *
   * @return
   *
   * A Decoder instance or nullptr if a Decoder with this ID does not exist
   */
  static DecoderPtr CreateFromID(const QString& id);

protected:
  bool open_;

private:
  StreamPtr stream_;
};

#endif // DECODER_H
