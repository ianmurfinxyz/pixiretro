#ifndef _LOG_H_  
#define _LOG_H_  

namespace pxr
{
namespace log
{

// log log strings.

constexpr const char* msg_log_fail_open = "failed to open log file";
constexpr const char* msg_log_to_stderr = "logging to standard error";

// engine log strings.

constexpr const char* msg_eng_fail_sdl_init = "failed to initialize SDL";
constexpr const char* msg_eng_locking_fps = "locking fps to";
constexpr const char* msg_eng_fail_load_splash = "failed to splash sprite : skipping splash screen";

// gfx log strings.

constexpr const char* msg_gfx_initializing = "initializing gfx module";
constexpr const char* msg_gfx_fail_init = "failed to initialize gfx module : terminating program";
constexpr const char* msg_gfx_fullscreen = "activating fullscreen window mode";
constexpr const char* msg_gfx_creating_window = "creating window";
constexpr const char* msg_gfx_fail_create_window = "failed to create window";
constexpr const char* msg_gfx_created_window = "successfully created window";
constexpr const char* msg_gfx_fail_create_opengl_context = "failed to create opengl context";
constexpr const char* msg_gfx_fail_set_opengl_attribute = "failed to set opengl attribute";
constexpr const char* msg_gfx_opengl_version = "using opengl version";
constexpr const char* msg_gfx_opengl_renderer = "using opengl renderer";
constexpr const char* msg_gfx_opengl_vendor = "using opengl vendor";
constexpr const char* msg_gfx_loading_sprites = "starting sprite loading";
constexpr const char* msg_gfx_loading_sprite = "loading sprite";
constexpr const char* msg_gfx_sprite_already_loaded = "sprite already loaded";
constexpr const char* msg_gfx_loading_sprite_success = "successfully loaded sprite";
constexpr const char* msg_gfx_loading_font = "loading font";
constexpr const char* msg_gfx_loading_font_success = "successfully loaded font";
constexpr const char* msg_gfx_fail_load_asset_bmp = "failed to load the bitmap image of asset";
constexpr const char* msg_gfx_using_error_sprite = "substituting unloaded sprite with error sprite";
constexpr const char* msg_gfx_using_error_font = "substituting unloaded font with error font";
constexpr const char* msg_gfx_loading_fonts = "starting font loading";
constexpr const char* msg_gfx_pixel_size_range = "range of valid pixel sizes";
constexpr const char* msg_gfx_created_vscreen = "created vscreen";
constexpr const char* msg_gfx_missing_ascii_glyphs = "loaded font does not contain glyphs for all 95 printable ascii chars";
constexpr const char* msg_gfx_font_fail_checksum = "loaded font failed the checksum test; may be duplicate ascii chars";
constexpr const char* msg_gfx_sprite_invalid_xml_bmp_mismatch = "invalid sprite : xml data implies a different bitmap size";
constexpr const char* msg_gfx_font_invalid_xml_bmp_mismatch = "invalid font : char xml meta extends font bmp bounds";
constexpr const char* msg_gfx_unloading_nonexistent_resource = "trying to unload nonexistent resource";
constexpr const char* msg_gfx_unload_sprite_success = "successfully unloaded sprite";
constexpr const char* msg_gfx_unload_font_success = "successfully unloaded font";

// sfx log strings.

constexpr const char* msg_sfx_initializing = "initializing sfx module";
constexpr const char* msg_sfx_fail_init = "failed to initialize sfx module : terminating program";
constexpr const char* msg_sfx_listing_devices = "listing sound devices : [<device-id>] : <device-name>";
constexpr const char* msg_sfx_device = "device";
constexpr const char* msg_sfx_default_device = "default device";
constexpr const char* msg_sfx_set_device_info = "modify enginerc to select an alternate sound device";
constexpr const char* msg_sfx_invalid_deviceid = "deviceid invalid : no such device";
constexpr const char* msg_sfx_creating_device = "creating openAL sound device";
constexpr const char* msg_sfx_fail_create_device = "failed to create openAL sound device";
constexpr const char* msg_sfx_fail_create_context = "failed to create openAL sound context";
constexpr const char* msg_sfx_openal_error = "openAL error";
constexpr const char* msg_sfx_openal_call = "openAL call";
constexpr const char* msg_sfx_playing_nonexistent_sound = "trying to play nonexistent sound";
constexpr const char* msg_sfx_using_error_sound = "substituting unloaded sound with error sound";
constexpr const char* msg_sfx_no_free_sources = "cannot play sound as no free sources";
constexpr const char* msg_sfx_loading_sound = "loading sound";
constexpr const char* msg_sfx_sound_already_loaded = "sound already loaded";
constexpr const char* msg_sfx_unloading_nonexistent_sound = "trying to unload nonexistent sound";
constexpr const char* msg_sfx_load_sound_success = "successfully loaded sound";
constexpr const char* msg_sfx_unload_sound_success = "successfully unloaded sound";


// xml log strings.

constexpr const char* msg_xml_parsing = "parsing xml asset file";
constexpr const char* msg_xml_fail_parse = "parsing error in xml file";
constexpr const char* msg_xml_fail_read_attribute = "failed to read xml attribute";
constexpr const char* msg_xml_fail_read_element = "failed to find xml element";
constexpr const char* msg_xml_tinyxml_error_name = "tinyxml2 error name";
constexpr const char* msg_xml_tinyxml_error_desc = "tinyxml2 error desc";

// cutscene log strings.

constexpr const char* msg_cut_loading = "loading cutscene";

// bitmap (bmp) file log strings.

constexpr const char* msg_bmp_fail_open = "failed to open bitmap image file";
constexpr const char* msg_bmp_corrupted = "expected a bitmap image file; file corrupted or wrong type";
constexpr const char* msg_bmp_unsupported_colorspace = "loaded bitmap image using unsupported non-sRGB color space";
constexpr const char* msg_bmp_unsupported_compression = "loaded bitmap image using unsupported compression mode";
constexpr const char* msg_bmp_unsupported_size = "loaded bitmap image has unsupported size";

// wavesound (wav) file log strings.

constexpr const char* msg_wav_loading = "loading wave sound file";
constexpr const char* msg_wav_fail_open = "failed to open wave sound file";
constexpr const char* msg_wav_read_fail = "failed to read data from a wave sound file";
constexpr const char* msg_wav_not_riff = "file not a riff file";
constexpr const char* msg_wav_not_wave = "file not a wave file";
constexpr const char* msg_wav_fmt_chunk_missing = "missing format chunk";
constexpr const char* msg_wav_not_pcm = "detected non-pcm sound data in wave : unsupported";
constexpr const char* msg_wav_bad_compressed = "detected compressed pcm data in wave : unsupported";
constexpr const char* msg_wav_odd_channels = "detected unsupported number of sound channels";
constexpr const char* msg_wav_odd_sample_bits = "detected unsupported number of bits per sample";
constexpr const char* msg_wav_data_chunk_missing = "missing data chunk";
constexpr const char* msg_wav_odd_data_size = "detected unsupported wave file size";
constexpr const char* msg_wav_load_success = "successfully loaded wave file";

// rcfile log strings.

constexpr const char* msg_rcfile_fail_open = "failed to open an rc file";
constexpr const char* msg_rcfile_fail_create = "failed to create an rc file";
constexpr const char* msg_rcfile_using_default = "using property default values";
constexpr const char* msg_rcfile_malformed = "malformed rc file";
constexpr const char* msg_rcfile_excess_seperators = "expected format <name><seperator><value>: seperators found:";
constexpr const char* msg_rcfile_malformed_property = "expected format <name><seperator><value>: missing key or value";
constexpr const char* msg_rcfile_unknown_property = "unknown property";
constexpr const char* msg_rcfile_expected_int = "expected integer value but found";
constexpr const char* msg_rcfile_expected_float = "expected float value but found";
constexpr const char* msg_rcfile_expected_bool = "expected bool value but found";
constexpr const char* msg_rcfile_property_clamped = "property value clamped to min-max range";
constexpr const char* msg_rcfile_property_read_success = "successfully read property";
constexpr const char* msg_rcfile_property_not_set = "property not set";
constexpr const char* msg_rcfile_errors = "found errors in rc file: error count";
constexpr const char* msg_rcfile_using_property_default = "using property default value";

// generic log strings.

constexpr const char* msg_on_line = "on line";
constexpr const char* msg_on_row = "on row";
constexpr const char* msg_ignoring_line = "ignoring line";

constexpr const char* msg_font_already_loaded = "font already loaded";
constexpr const char* msg_cannot_open_asset = "failed to open asset file";
constexpr const char* msg_asset_parse_errors = "asset file parsing errors";


enum Level
{
  FATAL,
  ERROR,
  WARN,
  INFO
};

void initialize();
void shutdown();

void log(Level level, const char* error, const std::string& addendum = std::string{});

} // namespace log
} // namespace pxr

#endif
