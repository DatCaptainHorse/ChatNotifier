module;

#include <miniaudio.h>
#include <opusfile.h>

#include <cstdlib>
#include <cstring>

export module opus_decoder;

using ma_libopus = struct {
	ma_data_source_base ds; /* The libopus decoder can be used independently as a data source. */
	ma_read_proc onRead;
	ma_seek_proc onSeek;
	ma_tell_proc onTell;
	void *pReadSeekTellUserData;
	ma_format format; /* Will be either f32 or s16. */
	OggOpusFile *of;
};

auto ma_libopus_init(ma_read_proc onRead, ma_seek_proc onSeek, ma_tell_proc onTell,
					 void *pReadSeekTellUserData, const ma_decoding_backend_config *pConfig,
					 const ma_allocation_callbacks *pAllocationCallbacks, ma_libopus *pOpus)
	-> ma_result;
auto ma_libopus_init_file(const char *pFilePath, const ma_decoding_backend_config *pConfig,
						  const ma_allocation_callbacks *pAllocationCallbacks, ma_libopus *pOpus)
	-> ma_result;
void ma_libopus_uninit(ma_libopus *pOpus);
auto ma_libopus_read_pcm_frames(ma_libopus *pOpus, void *pFramesOut, ma_uint64 frameCount,
								ma_uint64 *pFramesRead) -> ma_result;
auto ma_libopus_seek_to_pcm_frame(const ma_libopus *pOpus, ma_uint64 frameIndex) -> ma_result;
auto ma_libopus_get_data_format(const ma_libopus *pOpus, ma_format *pFormat, ma_uint32 *pChannels,
								ma_uint32 *pSampleRate, ma_channel *pChannelMap,
								size_t channelMapCap) -> ma_result;
auto ma_libopus_get_cursor_in_pcm_frames(const ma_libopus *pOpus, ma_uint64 *pCursor) -> ma_result;
auto ma_libopus_get_length_in_pcm_frames(const ma_libopus *pOpus, ma_uint64 *pLength) -> ma_result;

auto ma_libopus_ds_read(ma_data_source *pDataSource, void *pFramesOut, const ma_uint64 frameCount,
						ma_uint64 *pFramesRead) -> ma_result {
	return ma_libopus_read_pcm_frames(static_cast<ma_libopus *>(pDataSource), pFramesOut,
									  frameCount, pFramesRead);
}

auto ma_libopus_ds_seek(ma_data_source *pDataSource, const ma_uint64 frameIndex) -> ma_result {
	return ma_libopus_seek_to_pcm_frame(static_cast<ma_libopus *>(pDataSource), frameIndex);
}

auto ma_libopus_ds_get_data_format(ma_data_source *pDataSource, ma_format *pFormat,
								   ma_uint32 *pChannels, ma_uint32 *pSampleRate,
								   ma_channel *pChannelMap, const size_t channelMapCap)
	-> ma_result {
	return ma_libopus_get_data_format(static_cast<ma_libopus *>(pDataSource), pFormat, pChannels,
									  pSampleRate, pChannelMap, channelMapCap);
}

auto ma_libopus_ds_get_cursor(ma_data_source *pDataSource, ma_uint64 *pCursor) -> ma_result {
	return ma_libopus_get_cursor_in_pcm_frames(static_cast<ma_libopus *>(pDataSource), pCursor);
}

auto ma_libopus_ds_get_length(ma_data_source *pDataSource, ma_uint64 *pLength) -> ma_result {
	return ma_libopus_get_length_in_pcm_frames(static_cast<ma_libopus *>(pDataSource), pLength);
}

ma_data_source_vtable g_ma_libopus_ds_vtable = {ma_libopus_ds_read, ma_libopus_ds_seek,
												ma_libopus_ds_get_data_format,
												ma_libopus_ds_get_cursor, ma_libopus_ds_get_length};

auto ma_libopus_of_callback_read(void *pUserData, unsigned char *pBufferOut, const int bytesToRead)
	-> int {
	const auto *pOpus = static_cast<ma_libopus *>(pUserData);
	size_t bytesRead;

	if (const ma_result result = pOpus->onRead(
			pOpus->pReadSeekTellUserData, static_cast<void *>(pBufferOut), bytesToRead, &bytesRead);
		result != MA_SUCCESS) {
		return -1;
	}

	return static_cast<int>(bytesRead);
}

auto ma_libopus_of_callback_seek(void *pUserData, const ogg_int64_t offset, const int whence)
	-> int {
	const auto *pOpus = static_cast<ma_libopus *>(pUserData);
	ma_seek_origin origin;

	if (whence == SEEK_SET) {
		origin = ma_seek_origin_start;
	} else if (whence == SEEK_END) {
		origin = ma_seek_origin_end;
	} else {
		origin = ma_seek_origin_current;
	}

	const ma_result result = pOpus->onSeek(pOpus->pReadSeekTellUserData, offset, origin);
	if (result != MA_SUCCESS) {
		return -1;
	}

	return 0;
}

auto ma_libopus_of_callback_tell(void *pUserData) -> opus_int64 {
	const auto *pOpus = static_cast<ma_libopus *>(pUserData);
	ma_int64 cursor;

	if (pOpus->onTell == nullptr) {
		return -1;
	}

	const ma_result result = pOpus->onTell(pOpus->pReadSeekTellUserData, &cursor);
	if (result != MA_SUCCESS) {
		return -1;
	}

	return cursor;
}

auto ma_libopus_init_internal(const ma_decoding_backend_config *pConfig, ma_libopus *pOpus)
	-> ma_result {

	if (pOpus == nullptr) {
		return MA_INVALID_ARGS;
	}

	memset(pOpus, 0, sizeof(*pOpus));
	pOpus->format = ma_format_f32; /* f32 by default. */

	if (pConfig != nullptr &&
		(pConfig->preferredFormat == ma_format_f32 || pConfig->preferredFormat == ma_format_s16)) {
		pOpus->format = pConfig->preferredFormat;
	} else {
		/* Getting here means something other than f32 and s16 was specified. Just leave this unset
		 * to use the default format. */
	}

	ma_data_source_config dataSourceConfig = ma_data_source_config_init();
	dataSourceConfig.vtable = &g_ma_libopus_ds_vtable;

	const ma_result result = ma_data_source_init(&dataSourceConfig, &pOpus->ds);
	if (result != MA_SUCCESS) {
		return result; /* Failed to initialize the base data source. */
	}

	return MA_SUCCESS;
}

auto ma_libopus_init(const ma_read_proc onRead, const ma_seek_proc onSeek,
					 const ma_tell_proc onTell, void *pReadSeekTellUserData,
					 const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *,
					 ma_libopus *pOpus) -> ma_result {
	const ma_result result = ma_libopus_init_internal(pConfig, pOpus);
	if (result != MA_SUCCESS) {
		return result;
	}

	if (onRead == nullptr || onSeek == nullptr) {
		return MA_INVALID_ARGS; /* onRead and onSeek are mandatory. */
	}

	pOpus->onRead = onRead;
	pOpus->onSeek = onSeek;
	pOpus->onTell = onTell;
	pOpus->pReadSeekTellUserData = pReadSeekTellUserData;

	{
		int libopusResult;
		OpusFileCallbacks libopusCallbacks;

		/* We can now initialize the Opus decoder. This must be done after we've set up the
		 * callbacks. */
		libopusCallbacks.read = ma_libopus_of_callback_read;
		libopusCallbacks.seek = ma_libopus_of_callback_seek;
		libopusCallbacks.close = nullptr;
		libopusCallbacks.tell = ma_libopus_of_callback_tell;

		pOpus->of = op_open_callbacks(pOpus, &libopusCallbacks, nullptr, 0, &libopusResult);
		if (pOpus->of == nullptr) {
			return MA_INVALID_FILE;
		}

		return MA_SUCCESS;
	}
}

auto ma_libopus_init_file(const char *pFilePath, const ma_decoding_backend_config *pConfig,
						  const ma_allocation_callbacks *, ma_libopus *pOpus) -> ma_result {
	const ma_result result = ma_libopus_init_internal(pConfig, pOpus);
	if (result != MA_SUCCESS) {
		return result;
	}

	{
		int libopusResult;

		pOpus->of = op_open_file(pFilePath, &libopusResult);
		if (pOpus->of == nullptr) {
			return MA_INVALID_FILE;
		}

		return MA_SUCCESS;
	}
}

void ma_libopus_uninit(ma_libopus *pOpus) {
	if (!pOpus)
		return;

	op_free(pOpus->of);
	ma_data_source_uninit(&pOpus->ds);
}

auto ma_libopus_read_pcm_frames(ma_libopus *pOpus, void *pFramesOut, const ma_uint64 frameCount,
								ma_uint64 *pFramesRead) -> ma_result {
	if (pFramesRead != nullptr) {
		*pFramesRead = 0;
	}

	if (frameCount == 0) {
		return MA_INVALID_ARGS;
	}

	if (pOpus == nullptr) {
		return MA_INVALID_ARGS;
	}

	{
		/* We always use floating point format. */
		ma_result result = MA_SUCCESS; /* Must be initialized to MA_SUCCESS. */
		ma_format format;
		ma_uint32 channels;

		ma_libopus_get_data_format(pOpus, &format, &channels, nullptr, nullptr, 0);

		ma_uint64 totalFramesRead = 0;
		while (totalFramesRead < frameCount) {
			long libopusResult;

			const ma_uint64 framesRemaining = (frameCount - totalFramesRead);
			int framesToRead = 1024;
			if (framesToRead > framesRemaining) {
				framesToRead = static_cast<int>(framesRemaining);
			}

			if (format == ma_format_f32) {
				libopusResult = op_read_float(pOpus->of,
											  static_cast<float *>(ma_offset_pcm_frames_ptr(
												  pFramesOut, totalFramesRead, format, channels)),
											  static_cast<int>(framesToRead * channels), nullptr);
			} else {
				libopusResult = op_read(pOpus->of,
										static_cast<opus_int16 *>(ma_offset_pcm_frames_ptr(
											pFramesOut, totalFramesRead, format, channels)),
										static_cast<int>(framesToRead * channels), nullptr);
			}

			if (libopusResult < 0) {
				result = MA_ERROR; /* Error while decoding. */
				break;
			} else {
				totalFramesRead += libopusResult;

				if (libopusResult == 0) {
					result = MA_AT_END;
					break;
				}
			}
		}

		if (pFramesRead != nullptr) {
			*pFramesRead = totalFramesRead;
		}

		if (result == MA_SUCCESS && totalFramesRead == 0) {
			result = MA_AT_END;
		}

		return result;
	}
}

auto ma_libopus_seek_to_pcm_frame(const ma_libopus *pOpus, const ma_uint64 frameIndex)
	-> ma_result {
	if (pOpus == nullptr) {
		return MA_INVALID_ARGS;
	}

	{
		const int libopusResult = op_pcm_seek(pOpus->of, static_cast<ogg_int64_t>(frameIndex));
		if (libopusResult != 0) {
			if (libopusResult == OP_ENOSEEK) {
				return MA_INVALID_OPERATION; /* Not seekable. */
			} else if (libopusResult == OP_EINVAL) {
				return MA_INVALID_ARGS;
			} else {
				return MA_ERROR;
			}
		}

		return MA_SUCCESS;
	}
}

auto ma_libopus_get_data_format(const ma_libopus *pOpus, ma_format *pFormat, ma_uint32 *pChannels,
								ma_uint32 *pSampleRate, ma_channel *pChannelMap,
								const size_t channelMapCap) -> ma_result {
	/* Defaults for safety. */
	if (pFormat != nullptr) {
		*pFormat = ma_format_unknown;
	}
	if (pChannels != nullptr) {
		*pChannels = 0;
	}
	if (pSampleRate != nullptr) {
		*pSampleRate = 0;
	}
	if (pChannelMap != nullptr) {
		memset(pChannelMap, 0, channelMapCap * sizeof(*pChannelMap));
	}

	if (pOpus == nullptr) {
		return MA_INVALID_OPERATION;
	}

	if (pFormat != nullptr) {
		*pFormat = pOpus->format;
	}

	{
		const ma_uint32 channels = op_channel_count(pOpus->of, -1);

		if (pChannels != nullptr) {
			*pChannels = channels;
		}

		if (pSampleRate != nullptr) {
			*pSampleRate = 48000;
		}

		if (pChannelMap != nullptr) {
			ma_channel_map_init_standard(ma_standard_channel_map_vorbis, pChannelMap, channelMapCap,
										 channels);
		}

		return MA_SUCCESS;
	}
}

auto ma_libopus_get_cursor_in_pcm_frames(const ma_libopus *pOpus, ma_uint64 *pCursor) -> ma_result {
	if (pCursor == nullptr) {
		return MA_INVALID_ARGS;
	}

	*pCursor = 0; /* Safety. */

	if (pOpus == nullptr) {
		return MA_INVALID_ARGS;
	}

	{
		const ogg_int64_t offset = op_pcm_tell(pOpus->of);
		if (offset < 0) {
			return MA_INVALID_FILE;
		}

		*pCursor = static_cast<ma_uint64>(offset);

		return MA_SUCCESS;
	}
}

auto ma_libopus_get_length_in_pcm_frames(const ma_libopus *pOpus, ma_uint64 *pLength) -> ma_result {
	if (pLength == nullptr) {
		return MA_INVALID_ARGS;
	}

	*pLength = 0; /* Safety. */

	if (pOpus == nullptr) {
		return MA_INVALID_ARGS;
	}

	{
		const ogg_int64_t length = op_pcm_total(pOpus->of, -1);
		if (length < 0) {
			return MA_ERROR;
		}

		*pLength = static_cast<ma_uint64>(length);

		return MA_SUCCESS;
	}
}

export auto ma_decoding_backend_init_libopus(void *, const ma_read_proc onRead,
											 const ma_seek_proc onSeek, const ma_tell_proc onTell,
											 void *pReadSeekTellUserData,
											 const ma_decoding_backend_config *pConfig,
											 const ma_allocation_callbacks *,
											 ma_data_source **ppBackend) -> ma_result {
	const auto pOpus = new ma_libopus();
	if (!pOpus) {
		return MA_OUT_OF_MEMORY;
	}

	if (const ma_result result =
			ma_libopus_init(onRead, onSeek, onTell, pReadSeekTellUserData, pConfig, nullptr, pOpus);
		result != MA_SUCCESS) {
		return result;
	}

	*ppBackend = pOpus;

	return MA_SUCCESS;
}

export auto ma_decoding_backend_init_file_libopus(void *, const char *pFilePath,
												  const ma_decoding_backend_config *pConfig,
												  const ma_allocation_callbacks *,
												  ma_data_source **ppBackend) -> ma_result {
	const auto pOpus = new ma_libopus();
	if (!pOpus) {
		return MA_OUT_OF_MEMORY;
	}

	if (const ma_result result = ma_libopus_init_file(pFilePath, pConfig, nullptr, pOpus);
		result != MA_SUCCESS) {
		return result;
	}

	*ppBackend = pOpus;

	return MA_SUCCESS;
}

export void ma_decoding_backend_uninit_libopus(void *, ma_data_source *pBackend,
											   const ma_allocation_callbacks *) {
	auto *pOpus = static_cast<ma_libopus *>(pBackend);
	ma_libopus_uninit(pOpus);
	free(pOpus);
}
