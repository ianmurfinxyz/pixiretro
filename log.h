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

constexpr const char* msg_fail_sdl_init = "failed to initialize SDL";

// gfx log strings.

constexpr const char* msg_gfx_creating_window = "creating window";
constexpr const char* msg_gfx_fail_create_window = "failed to create window";
constexpr const char* msg_gfx_created_window = "successfully created window";
constexpr const char* msg_gfx_fail_create_opengl_context = "failed to create opengl context";
constexpr const char* msg_gfx_fail_set_opengl_attribute = "failed to set opengl attribute";
constexpr const char* msg_gfx_opengl_version = "using opengl version";
constexpr const char* msg_gfx_fail_load_sprite = "failed to load a sprite asset";
constexpr const char* msg_gfx_using_error_sprite = "susbtituting unloaded sprite with 8x8 red square";
constexpr const char* msg_gfx_duplicate_resource_key = "asset manifest contains duplicate resource key";
constexpr const char* msg_gfx_skipping_asset_load = "skipping asset loading";
constexpr const char* msg_gfx_fullscreen = "activating fullscreen window mode";
constexpr const char* msg_gfx_pixel_size_range = "range of valid pixel sizes";
constexpr const char* msg_gfx_created_vscreen = "created vscreen";

// bitmap (bmp) file log strings.

constexpr const char* msg_bmp_fail_open = "failed to open bitmap image file";
constexpr const char* msg_bmp_corrupted = "expected a bitmap image file; file corrupted or wrong type";
constexpr const char* msg_bmp_unsupported_colorspace = "loaded bitmap image using unsupported non-sRGB color space";
constexpr const char* msg_bmp_unsupported_compression = "loaded bitmap image using unsupported compression mode";
constexpr const char* msg_bmp_unsupported_size = "loaded bitmap image has unsupported size";

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
