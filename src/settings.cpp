#include "settings.hpp"

#include "paths.hpp"
#include "schema.hpp"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <valijson/adapters/nlohmann_json_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

namespace dictate {

Settings g_settings;

std::filesystem::path config_path() { return exe_dir() / "config.json"; }

const char *model_filename(const Settings::Model model) {
  switch (model) {
  case Settings::Model::TinyEn:
    return "ggml-tiny.en.bin";
  case Settings::Model::BaseEn:
    return "ggml-base.en.bin";
  case Settings::Model::SmallEn:
    return "ggml-small.en.bin";
  case Settings::Model::MediumEn:
    return "ggml-medium.en.bin";
  case Settings::Model::LargeV3:
    return "ggml-large-v3.bin";
  }
  return "ggml-tiny.en.bin";
}

namespace {

bool validate_config(const nlohmann::json &doc, std::string &error) {
  const nlohmann::json schema = nlohmann::json::parse(kConfigSchema);

  valijson::Schema schema_obj;
  valijson::SchemaParser parser;
  valijson::adapters::NlohmannJsonAdapter schema_adapter(schema);
  try {
    parser.populateSchema(schema_adapter, schema_obj);
  } catch (const std::exception &e) {
    error = e.what();
    return false;
  }

  valijson::Validator validator;
  valijson::adapters::NlohmannJsonAdapter doc_adapter(doc);
  valijson::ValidationResults results;
  if (validator.validate(schema_obj, doc_adapter, &results))
    return true;

  valijson::ValidationResults::Error err;
  unsigned n = 0;
  while (results.popError(err)) {
    if (n++)
      error += "; ";
    error += err.description;
  }
  return false;
}

std::optional<Settings::Model> parse_model(const std::string &name) {
  if (name == "tiny.en")
    return Settings::Model::TinyEn;
  if (name == "base.en")
    return Settings::Model::BaseEn;
  if (name == "small.en")
    return Settings::Model::SmallEn;
  if (name == "medium.en")
    return Settings::Model::MediumEn;
  if (name == "large-v3")
    return Settings::Model::LargeV3;
  return std::nullopt;
}

Settings settings_from_json(const nlohmann::json &j) {
  Settings s;
  s.mode = j["mode"] == "toggle" ? Settings::Mode::Toggle : Settings::Mode::Hold;
  s.hotkey = j["hotkey"].get<std::string>();
  s.model = *parse_model(j["model"].get<std::string>());
  return s;
}

} // namespace

std::optional<Settings> load_settings() {
  std::ifstream in(config_path());
  if (!in) {
    std::cerr << "Missing " << config_path() << '\n';
    return std::nullopt;
  }

  nlohmann::json j;
  try {
    in >> j;
  } catch (const nlohmann::json::exception &e) {
    std::cerr << "Invalid JSON in " << config_path() << ": " << e.what()
              << '\n';
    return std::nullopt;
  }

  std::string error;
  if (!validate_config(j, error)) {
    std::cerr << "Config validation failed: " << error << '\n';
    return std::nullopt;
  }

  return settings_from_json(j);
}

} // namespace dictate
