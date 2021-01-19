#ifndef _LOG_H_  
#define _LOG_H_  

namespace pxr
{
namespace log
{

constexpr const char* fail_open_log = "failed to open log";
constexpr const char* fail_sdl_init = "failed to initialize SDL";
constexpr const char* fail_create_opengl_context = "failed to create opengl context";
constexpr const char* fail_set_opengl_attribute = "failed to set opengl attribute";
constexpr const char* fail_create_window = "failed to create window";

constexpr const char* warn_cannot_open_dataset = "failed to open data file";
constexpr const char* warn_cannot_create_dataset = "failed to create data file";
constexpr const char* warn_malformed_dataset = "malformed data file";
constexpr const char* warn_property_not_set = "property not set";
constexpr const char* warn_malformed_bitmap = "malformed bitmap";
constexpr const char* warn_bitmap_already_loaded = "bitmap already loaded";
constexpr const char* warn_empty_bitmap_file = "empty bitmap file";
constexpr const char* warn_font_already_loaded = "font already loaded";
constexpr const char* warn_cannot_open_asset = "failed to open asset file";
constexpr const char* warn_asset_parse_errors = "asset file parsing errors";

constexpr const char* info_stderr_log = "logging to standard error";
constexpr const char* info_using_default_config = "using default engine configuration";
constexpr const char* info_fullscreen_mode = "activating fullscreen mode";
constexpr const char* info_creating_window = "creating window";
constexpr const char* info_created_window = "window created";
constexpr const char* info_using_opengl_version = "using opengl version";
constexpr const char* info_pixel_size_range = "range of valid pixel sizes";
constexpr const char* info_on_line = "on line";
constexpr const char* info_on_row = "on row";
constexpr const char* info_unknown_dataset_property = "unkown property";
constexpr const char* info_unexpected_seperators = "expected a single seperator character";
constexpr const char* info_incomplete_property = "missing property key or value";
constexpr const char* info_expected_integer = "expected integer value";
constexpr const char* info_expected_float = "expected float value";
constexpr const char* info_expected_bool = "expected bool value";
constexpr const char* info_ignoring_line = "ignoring line";
constexpr const char* info_property_set = "dataset property set";
constexpr const char* info_property_clamped = "property value clamped to min-max range";
constexpr const char* info_using_property_defaults = "using property default values";
constexpr const char* info_using_error_bitmap = "substituting with blank bitmap";
constexpr const char* info_using_error_font = "substituting with blank font";
constexpr const char* info_using_error_glyph = "substituting with blank glyph";
constexpr const char* info_loading_asset = "loading asset";
constexpr const char* info_skipping_asset_loading = "skipping asset loading";
constexpr const char* info_ascii_code = "ascii code";

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
