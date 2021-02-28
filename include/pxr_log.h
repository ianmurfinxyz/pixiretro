#ifndef _PIXIRETRO_LOG_H_  
#define _PIXIRETRO_LOG_H_  

#include <array>

namespace pxr
{
namespace log
{

#define LOGSTR constexpr const char*

//
// log log strings.
//

LOGSTR msg_log_fail_open = "failed to open log file";
LOGSTR msg_log_to_stderr = "logging to standard error";

//
// engine log strings.
//

LOGSTR msg_eng_fail_sdl_init = "failed to initialize SDL";
LOGSTR msg_eng_locking_fps = "locking fps to";
LOGSTR msg_eng_fail_load_splash = "failed to splash sprite : skipping splash screen";
LOGSTR msg_eng_fail_init_app = "failed to initialize the app";

//
// gfx log strings.
//

LOGSTR msg_gfx_initializing = "initializing gfx module";
LOGSTR msg_gfx_fail_init = "failed to initialize gfx module : terminating program";
LOGSTR msg_gfx_fullscreen = "activating fullscreen window mode";
LOGSTR msg_gfx_creating_window = "creating window";
LOGSTR msg_gfx_fail_create_window = "failed to create window";
LOGSTR msg_gfx_created_window = "successfully created window";
LOGSTR msg_gfx_fail_create_opengl_context = "failed to create opengl context";
LOGSTR msg_gfx_fail_set_opengl_attribute = "failed to set opengl attribute";
LOGSTR msg_gfx_opengl_version = "using opengl version";
LOGSTR msg_gfx_opengl_renderer = "using opengl renderer";
LOGSTR msg_gfx_opengl_vendor = "using opengl vendor";
LOGSTR msg_gfx_loading_spritesheets = "starting spritesheet loading";
LOGSTR msg_gfx_loading_spritesheet = "loading spritesheet";
LOGSTR msg_gfx_spritesheet_already_loaded = "spritesheet already loaded";
LOGSTR msg_gfx_loading_spritesheet_success = "successfully loaded spritesheet";
LOGSTR msg_gfx_loading_font = "loading font";
LOGSTR msg_gfx_loading_font_success = "successfully loaded font";
LOGSTR msg_gfx_fail_load_asset_bmp = "failed to load the bitmap image of asset";
LOGSTR msg_gfx_using_error_spritesheet = "substituting unloaded spritesheet with error spritesheet";
LOGSTR msg_gfx_using_error_font = "substituting unloaded font with error font";
LOGSTR msg_gfx_loading_fonts = "starting font loading";
LOGSTR msg_gfx_pixel_size_range = "range of valid pixel sizes";
LOGSTR msg_gfx_created_vscreen = "created vscreen";
LOGSTR msg_gfx_missing_ascii_glyphs = "loaded font does not contain glyphs for all 95 printable ascii chars";
LOGSTR msg_gfx_font_fail_checksum = "loaded font failed the checksum test; may be duplicate ascii chars";
LOGSTR msg_gfx_spritesheet_invalid_xml_bmp_mismatch = "invalid spritesheet : xml data implies a different bitmap size";
LOGSTR msg_gfx_font_invalid_xml_bmp_mismatch = "invalid font : char xml meta extends font bmp bounds";
LOGSTR msg_gfx_unloading_nonexistent_resource = "trying to unload nonexistent resource";
LOGSTR msg_gfx_unload_spritesheet_success = "successfully unloaded spritesheet";
LOGSTR msg_gfx_unload_font_success = "successfully unloaded font";

//
// sfx log strings.
//

LOGSTR msg_sfx_initializing = "initializing sfx module";
LOGSTR msg_sfx_fail_init = "failed to initialize sfx module : terminating program";
LOGSTR msg_sfx_listing_devices = "listing sound devices : [<device-id>] : <device-name>";
LOGSTR msg_sfx_device = "device";
LOGSTR msg_sfx_default_device = "default device";
LOGSTR msg_sfx_set_device_info = "modify enginerc to select an alternate sound device";
LOGSTR msg_sfx_invalid_deviceid = "deviceid invalid : no such device";
LOGSTR msg_sfx_creating_device = "creating openAL sound device";
LOGSTR msg_sfx_fail_create_device = "failed to create openAL sound device";
LOGSTR msg_sfx_fail_create_context = "failed to create openAL sound context";
LOGSTR msg_sfx_openal_error = "openAL error";
LOGSTR msg_sfx_openal_call = "openAL call";
LOGSTR msg_sfx_playing_nonexistent_sound = "trying to play nonexistent sound";
LOGSTR msg_sfx_using_error_sound = "substituting unloaded sound with error sound";
LOGSTR msg_sfx_no_free_sources = "cannot play sound as no free sources";
LOGSTR msg_sfx_loading_sound = "loading sound";
LOGSTR msg_sfx_sound_already_loaded = "sound already loaded";
LOGSTR msg_sfx_unloading_nonexistent_sound = "trying to unload nonexistent sound";
LOGSTR msg_sfx_load_sound_success = "successfully loaded sound";
LOGSTR msg_sfx_unload_sound_success = "successfully unloaded sound";


//
// xml log strings.
//

LOGSTR msg_xml_parsing = "parsing xml asset file";
LOGSTR msg_xml_fail_parse = "parsing error in xml file";
LOGSTR msg_xml_fail_read_attribute = "failed to read xml attribute";
LOGSTR msg_xml_fail_read_element = "failed to find xml element";
LOGSTR msg_xml_tinyxml_error_name = "tinyxml2 error name";
LOGSTR msg_xml_tinyxml_error_desc = "tinyxml2 error desc";

//
// cutscene log strings.
//

LOGSTR msg_cut_loading = "loading cutscene";

//
// bmp log strings.
//

LOGSTR msg_bmp_fail_open = "failed to open bitmap image file";
LOGSTR msg_bmp_corrupted = "expected a bitmap image file; file corrupted or wrong type";
LOGSTR msg_bmp_unsupported_colorspace = "loaded bitmap image using unsupported non-sRGB color space";
LOGSTR msg_bmp_unsupported_compression = "loaded bitmap image using unsupported compression mode";
LOGSTR msg_bmp_unsupported_size = "loaded bitmap image has unsupported size";

//
// wav file log strings.
//

LOGSTR msg_wav_loading = "loading wave sound file";
LOGSTR msg_wav_fail_open = "failed to open wave sound file";
LOGSTR msg_wav_read_fail = "failed to read data from a wave sound file";
LOGSTR msg_wav_not_riff = "file not a riff file";
LOGSTR msg_wav_not_wave = "file not a wave file";
LOGSTR msg_wav_fmt_chunk_missing = "missing format chunk";
LOGSTR msg_wav_not_pcm = "detected non-pcm sound data in wave : unsupported";
LOGSTR msg_wav_bad_compressed = "detected compressed pcm data in wave : unsupported";
LOGSTR msg_wav_odd_channels = "detected unsupported number of sound channels";
LOGSTR msg_wav_odd_sample_bits = "detected unsupported number of bits per sample";
LOGSTR msg_wav_data_chunk_missing = "missing data chunk";
LOGSTR msg_wav_odd_data_size = "detected unsupported wave file size";
LOGSTR msg_wav_load_success = "successfully loaded wave file";

//
// rc log strings.
//

LOGSTR msg_rcfile_fail_open = "failed to open an rc file";
LOGSTR msg_rcfile_fail_create = "failed to create an rc file";
LOGSTR msg_rcfile_using_default = "using property default values";
LOGSTR msg_rcfile_malformed = "malformed rc file";
LOGSTR msg_rcfile_excess_seperators = "expected format <name><seperator><value>: seperators found:";
LOGSTR msg_rcfile_malformed_property = "expected format <name><seperator><value>: missing key or value";
LOGSTR msg_rcfile_unknown_property = "unknown property";
LOGSTR msg_rcfile_expected_int = "expected integer value but found";
LOGSTR msg_rcfile_expected_float = "expected float value but found";
LOGSTR msg_rcfile_expected_bool = "expected bool value but found";
LOGSTR msg_rcfile_property_clamped = "property value clamped to min-max range";
LOGSTR msg_rcfile_property_read_success = "successfully read property";
LOGSTR msg_rcfile_property_not_set = "property not set";
LOGSTR msg_rcfile_errors = "found errors in rc file: error count";
LOGSTR msg_rcfile_using_property_default = "using property default value";

// generic log strings.

LOGSTR msg_on_line = "on line";
LOGSTR msg_on_row = "on row";
LOGSTR msg_ignoring_line = "ignoring line";

LOGSTR msg_font_already_loaded = "font already loaded";
LOGSTR msg_cannot_open_asset = "failed to open asset file";
LOGSTR msg_asset_parse_errors = "asset file parsing errors";


//
// The name of the log file to open/create and write to on the filesystem.
//
static constexpr const char* LOG_FILENAME {"log"};

//
// The delimiter to divide log string sections.
//
static constexpr const char* LOG_DELIM {" : "};

//
// The log level controls the prefix to the actual string printed to the log.
//
enum Level
{
  FATAL,
  ERROR,
  WARN,
  INFO
};

//
// The log prefixes to output for each log level.
//
static constexpr std::array<const char*, 4> prefix {"Fatal", "Error", "Warning", "Info"};

//
// Initialises the log by opening the log stream. The first stream preference is a log 
// file with the name defined by LOG_FILENAME. The second preference is stderr which will
// be used if the log file cannot be opened or created.
//
void initialize();

//
// Closes the log stream.
//
void shutdown();

//
// Logs a string to the log with the format:
//
//    <prefix><delim><error><delim><addendum>
//
// where the <prefix> is determined by the log level.
//
void log(Level level, const char* error, const std::string& addendum = std::string{});

} // namespace log
} // namespace pxr

#endif
