#ifndef _LOG_H_  
#define _LOG_H_  

namespace pxr
{
namespace log
{

constexpr const char* msg_open_log = "failed to open log";
constexpr const char* msg_sdl_init = "failed to initialize SDL";
constexpr const char* msg_create_opengl_context = "failed to create opengl context";
constexpr const char* msg_set_opengl_attribute = "failed to set opengl attribute";
constexpr const char* msg_create_window = "failed to create window";

constexpr const char* msg_filed_open_bmp = "failed to open bitmap image file";
constexpr const char* msg_expected_bmp = "expected a bitmap image file; file corrupted or wrong type";
constexpr const char* msg_unsupported_bmp_colorspace = "loaded bitmap image using unsupported non-sRGB color space";
constexpr const char* msg_unsupported_bmp_compression = "loaded bitmap image using unsupported compression mode";
constexpr const char* msg_failed_load_sprite = "failed to load a sprite asset";
constexpr const char* msg_using_error_sprite = "susbtituting unloaded sprite with 8x8 red square";
constexpr const char* msg_diplicate_resource_key = "asset manifest contains duplicate resource key";

constexpr const char* msg_cannot_open_dataset = "failed to open data file";
constexpr const char* msg_cannot_create_dataset = "failed to create data file";
constexpr const char* msg_malformed_dataset = "malformed data file";
constexpr const char* msg_property_not_set = "property not set";
constexpr const char* msg_font_already_loaded = "font already loaded";
constexpr const char* msg_cannot_open_asset = "failed to open asset file";
constexpr const char* msg_asset_parse_errors = "asset file parsing errors";

constexpr const char* msg_stderr_log = "logging to standard error";
constexpr const char* msg_using_default_config = "using default engine configuration";
constexpr const char* msg_fullscreen_mode = "activating fullscreen mode";
constexpr const char* msg_creating_window = "creating window";
constexpr const char* msg_created_window = "window created";
constexpr const char* msg_using_opengl_version = "using opengl version";
constexpr const char* msg_pixel_size_range = "range of valid pixel sizes";
constexpr const char* msg_on_line = "on line";
constexpr const char* msg_on_row = "on row";
constexpr const char* msg_unknown_dataset_property = "unkown property";
constexpr const char* msg_unexpected_seperators = "expected a single seperator character";
constexpr const char* msg_incomplete_property = "missing property key or value";
constexpr const char* msg_expected_integer = "expected integer value";
constexpr const char* msg_expected_float = "expected float value";
constexpr const char* msg_expected_bool = "expected bool value";
constexpr const char* msg_ignoring_line = "ignoring line";
constexpr const char* msg_property_set = "dataset property set";
constexpr const char* msg_property_clamped = "property value clamped to min-max range";
constexpr const char* msg_using_property_defaults = "using property default values";
constexpr const char* msg_using_error_font = "substituting with blank font";
constexpr const char* msg_using_error_glyph = "substituting with blank glyph";
constexpr const char* msg_loading_asset = "loading asset";
constexpr const char* msg_skipping_asset_loading = "skipping asset loading";
constexpr const char* msg_ascii_code = "ascii code";

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
