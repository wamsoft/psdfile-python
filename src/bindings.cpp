#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <string>
#include <vector>
#include <fstream>
#include <memory>

#include "psdparse/psdfile.h" 
#include "psdparse/psdparse.h"


#include <string>
#include <cstdlib>

#include <boost/locale.hpp>

std::string wstring_to_string(const std::wstring& wstr) {
    return boost::locale::conv::utf_to_utf<char>(wstr);
}

namespace py = pybind11;

class PythonPSD : public psd::PSDFile {
private:
    std::vector<uint8_t> fileData; // メモリ上に保持するPSDファイルデータ

public:
    PythonPSD() {}
    ~PythonPSD() { clearData(); }

    void clearData() {
        psd::PSDFile::clearData();
        fileData.clear();
    }

    // ファイルからの読み込み
    bool loadFromFile(const std::string& filepath) {
        clearData();
        return psd::PSDFile::load(filepath.c_str());
    }

    // バイトデータからの読み込み
    bool loadFromBytes(py::bytes bytes) {
        clearData();
        
        // Python側のバイトデータをstd::vectorにコピー
        std::string buffer = static_cast<std::string>(bytes);
        fileData.assign(buffer.begin(), buffer.end());
        
        psd::Parser<uint8_t*> parser(*this);
        unsigned char *begin = fileData.data();
        size_t size = fileData.size();
        unsigned char *end   = begin + size;
        bool r = parse(begin , end,  parser);
        if (r && begin == end) {
            dprint("succeeded\n");
            isLoaded = processParsed();
        }
        if (!isLoaded) {
            clearData();
        }
        return isLoaded;
    }

    // 基本情報を辞書で取得
    py::dict getBasicInfo() const {
        py::dict info;
        if (!isLoaded) return info;

        info["width"] = header.width;
        info["height"] = header.height;
        info["channels"] = header.channels;
        info["depth"] = header.depth;
        info["color_mode"] = header.mode;
        info["layer_count"] = static_cast<int>(layerList.size());
        
        return info;
    }

    // レイヤーの種類を取得
    int getLayerType(int layerNo) const {
        if (!isLoaded || layerNo < 0 || layerNo >= static_cast<int>(layerList.size())) {
            throw std::runtime_error("Invalid layer number or no PSD data loaded");
        }
        return static_cast<int>(layerList[layerNo].layerType);
    }

    // レイヤー名を取得
    std::string getLayerName(int layerNo) const {
        if (!isLoaded || layerNo < 0 || layerNo >= static_cast<int>(layerList.size())) {
            throw std::runtime_error("Invalid layer number or no PSD data loaded");
        }
        
        const psd::LayerInfo& lay = layerList[layerNo];
        // Unicodeレイヤー名を優先
        if (!lay.layerNameUnicode.empty()) {
            return wstring_to_string(lay.layerNameUnicode);
        }
        return lay.layerName;
    }

    // レイヤー情報をPython辞書で取得
    py::dict getLayerInfo(int layerNo) const {
        py::dict result;
        if (!isLoaded || layerNo < 0 || layerNo >= static_cast<int>(layerList.size())) {
            throw std::runtime_error("Invalid layer number or no PSD data loaded");
        }

        const psd::LayerInfo& lay = layerList[layerNo];
        
        // 基本情報
        result["top"] = lay.top;
        result["left"] = lay.left;
        result["bottom"] = lay.bottom;
        result["right"] = lay.right;
        result["width"] = lay.width;
        result["height"] = lay.height;
        result["opacity"] = lay.opacity;
        result["fill_opacity"] = lay.fill_opacity;

        // マスク情報チェック
        bool hasMask = false;
        for (const auto& channel : lay.channels) {
            if (channel.isMaskChannel()) {
                hasMask = true;
                break;
            }
        }
        result["mask"] = hasMask;

        // ブレンドモード変換用ヘルパー関数
        auto convBlendModeToString = [](psd::BlendMode mode) -> std::string {
            // convBlendMode関数の実装（文字列版）
            switch (mode) {
                case psd::BLEND_MODE_NORMAL:      return "normal";
                case psd::BLEND_MODE_DARKEN:      return "darken";
                case psd::BLEND_MODE_MULTIPLY:    return "multiply";
                case psd::BLEND_MODE_COLOR_BURN:  return "color_burn";
                case psd::BLEND_MODE_LINEAR_BURN: return "linear_burn";
                case psd::BLEND_MODE_LIGHTEN:     return "lighten";
                case psd::BLEND_MODE_SCREEN:      return "screen";
                case psd::BLEND_MODE_COLOR_DODGE: return "color_dodge";
                case psd::BLEND_MODE_LINEAR_DODGE:return "linear_dodge";
                case psd::BLEND_MODE_OVERLAY:     return "overlay";
                case psd::BLEND_MODE_SOFT_LIGHT:  return "soft_light";
                case psd::BLEND_MODE_HARD_LIGHT:  return "hard_light";
                case psd::BLEND_MODE_DIFFERENCE:  return "difference";
                case psd::BLEND_MODE_EXCLUSION:   return "exclusion";
                default: return "normal";
            }
        };
        
        // ブレンドモードと種別
        result["type"] = convBlendModeToString(lay.blendMode);
        result["layer_type"] = static_cast<int>(lay.layerType);
        result["blend_mode"] = static_cast<int>(lay.blendMode);
        result["visible"] = lay.isVisible();
        result["name"] = getLayerName(layerNo);
        
        // 追加情報
        result["clipping"] = lay.clipping;
        result["layer_id"] = lay.layerId;
        result["obsolete"] = lay.isObsolete();
        result["transparency_protected"] = lay.isTransparencyProtected();
        result["pixel_data_irrelevant"] = lay.isPixelDataIrrelevant();
        
        // グループレイヤー情報
        if (lay.parent != nullptr) {
            result["group_layer_id"] = lay.parent->layerId;
        }
        
        // レイヤーカンプ情報
        if (!lay.layerComps.empty()) {
            py::dict compDict;
            for (const auto& comp_pair : lay.layerComps) {
                py::dict tmp;
                const psd::LayerCompInfo& comp = comp_pair.second;
                tmp["id"] = comp.id;
                tmp["offset_x"] = comp.offsetX;
                tmp["offset_y"] = comp.offsetY;
                tmp["enable"] = comp.isEnabled;
                compDict[py::cast(comp.id)] = tmp;
            }
            result["layer_comp"] = compDict;
        }
        
        return result;
    }

    // レイヤーデータをNumPy配列として取得
    py::array_t<uint8_t> getLayerData(int layerNo, const std::string& mode = "maskedimage") {
        if (!isLoaded || layerNo < 0 || layerNo >= static_cast<int>(layerList.size())) {
            throw std::runtime_error("Invalid layer number or no PSD data loaded");
        }

        const psd::LayerInfo& lay = layerList[layerNo];
        const psd::LayerMask& mask = lay.extraData.layerMask;
        
        int left, top, width, height;
        psd::ImageMode imageMode;
        
        if (mode == "mask") {
            // マスクのみ
            imageMode = psd::IMAGE_MODE_MASK;
            left = mask.left;
            top = mask.top;
            width = mask.width;
            height = mask.height;
            
            if (width <= 0 || height <= 0) {
                // ダミーのマスクを作成
                width = height = 1;
                py::array_t<uint8_t> dummyArray({1, 1, 4});
                auto buf = dummyArray.mutable_unchecked<3>();
                buf(0, 0, 0) = buf(0, 0, 1) = buf(0, 0, 2) = mask.defaultColor;
                buf(0, 0, 3) = 255;
                return dummyArray;
            }
        } else if (mode == "raw") {
            // 生イメージ
            imageMode = psd::IMAGE_MODE_IMAGE;
            left = lay.left;
            top = lay.top;
            width = lay.width;
            height = lay.height;
        } else {
            // マスク適用済み（デフォルト）
            imageMode = psd::IMAGE_MODE_MASKEDIMAGE;
            left = lay.left;
            top = lay.top;
            width = lay.width;
            height = lay.height;
        }
        
        if (width <= 0 || height <= 0) {
            throw std::runtime_error("Layer has zero width or height");
        }
        
        // BGRA形式でデータを取得するためのバッファを作成
        py::array_t<uint8_t> array({height, width, 4});
        auto buf = array.mutable_unchecked<3>();
        
        // 1行あたりのバイト数
        int pitch = width * 4;
        
        // 画像データを取得
        std::vector<uint8_t> buffer(pitch * height);
        getLayerImage(lay, buffer.data(), psd::BGRA_LE, pitch, imageMode);
        
        // NumPy配列にデータをコピー
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                for (int c = 0; c < 4; c++) {
                    buf(y, x, c) = buffer[y * pitch + x * 4 + c];
                }
            }
        }
        
        return array;
    }

    // 合成結果をNumPy配列として取得
    py::array_t<uint8_t> getBlend() {
        if (!isLoaded) {
            throw std::runtime_error("No PSD data loaded");
        }
        
        if (!imageData) {
            throw std::runtime_error("No composite image data available in the PSD file");
        }
        
        int width = header.width;
        int height = header.height;
        
        // BGRA形式でデータを取得するためのバッファを作成
        py::array_t<uint8_t> array({height, width, 4});
        auto buf = array.mutable_unchecked<3>();
        
        // 1行あたりのバイト数
        int pitch = width * 4;
        
        // 画像データを取得
        std::vector<uint8_t> buffer(pitch * height);
        getMergedImage(buffer.data(), psd::BGRA_LE, pitch);
        
        // NumPy配列にデータをコピー
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                for (int c = 0; c < 4; c++) {
                    buf(y, x, c) = buffer[y * pitch + x * 4 + c];
                }
            }
        }
        
        return array;
    }

    // スライス情報をPython辞書で取得
    py::object getSlices() {
        if (!isLoaded) {
            throw std::runtime_error("No PSD data loaded");
        }
        
        if (!slice.isEnabled) {
            return py::none();
        }
        
        py::dict result;
        psd::SliceResource& sr = slice;
        
        result["top"] = sr.boundingTop;
        result["left"] = sr.boundingLeft;
        result["bottom"] = sr.boundingBottom;
        result["right"] = sr.boundingRight;
        result["name"] = sr.groupName;
        
        py::list slicesList;
        for (const auto& item : sr.slices) {
            py::dict sliceDict;
            sliceDict["id"] = item.id;
            sliceDict["group_id"] = item.groupId;
            sliceDict["origin"] = item.origin;
            sliceDict["type"] = item.type;
            sliceDict["left"] = item.left;
            sliceDict["top"] = item.top;
            sliceDict["right"] = item.right;
            sliceDict["bottom"] = item.bottom;
            sliceDict["color"] = ((item.colorA << 24) | (item.colorR << 16) | (item.colorG << 8) | item.colorB);
            sliceDict["cell_text_is_html"] = item.isCellTextHtml;
            sliceDict["horizontal_alignment"] = item.horizontalAlign;
            sliceDict["vertical_alignment"] = item.verticalAlign;
            sliceDict["associated_layer_id"] = item.associatedLayerId;
            sliceDict["name"] = item.name;
            sliceDict["url"] = item.url;
            sliceDict["target"] = item.target;
            sliceDict["message"] = item.message;
            sliceDict["alt_tag"] = item.altTag;
            sliceDict["cell_text"] = item.cellText;
            
            slicesList.append(sliceDict);
        }
        
        result["slices"] = slicesList;
        return result;
    }

    // ガイド情報をPython辞書で取得
    py::object getGuides() {
        if (!isLoaded) {
            throw std::runtime_error("No PSD data loaded");
        }
        
        if (!gridGuide.isEnabled) {
            return py::none();
        }
        
        py::dict result;
        psd::GridGuideResource& gg = gridGuide;
        
        result["horz_grid"] = gg.horizontalGrid;
        result["vert_grid"] = gg.verticalGrid;
        
        py::list verticalGuides;
        py::list horizontalGuides;
        
        for (const auto& guide : gg.guides) {
            if (guide.direction == 0) {
                verticalGuides.append(guide.location);
            } else {
                horizontalGuides.append(guide.location);
            }
        }
        
        result["vertical"] = verticalGuides;
        result["horizontal"] = horizontalGuides;
        
        return result;
    }

    // レイヤーカンプ情報をPython辞書で取得
    py::object getLayerComp() {
        if (!isLoaded) {
            throw std::runtime_error("No PSD data loaded");
        }
        
        if (layerComps.empty()) {
            return py::none();
        }
        
        py::dict result;
        result["last_applied_id"] = lastAppliedCompId;
        
        py::list compsList;
        for (const auto& comp : layerComps) {
            py::dict compDict;
            compDict["id"] = comp.id;
            compDict["record_visibility"] = comp.isRecordVisibility;
            compDict["record_position"] = comp.isRecordPosition;
            compDict["record_appearance"] = comp.isRecordAppearance;
            compDict["name"] = comp.name;
            compDict["comment"] = comp.comment;
            
            compsList.append(compDict);
        }
        
        result["comps"] = compsList;
        return result;
    }

    // レイヤーIDの自動割り当て
    int assignAutoIds(int baseId = 0) {
        if (!isLoaded) {
            throw std::runtime_error("No PSD data loaded");
        }
        
        // 既存のレイヤーから最大IDを探す
        int minId = -1;
        for (const auto& layer : layerList) {
            if (minId < layer.layerId) {
                minId = layer.layerId;
            }
        }
        
        // base_idとの比較
        if (baseId < minId) {
            baseId = minId;
        }
        
        // layerId が -1 のレイヤーに対して新しいIDを割り当て
        int assigned = 0;
        for (auto& layer : layerList) {
            if (layer.layerId == -1) {
                layer.layerId = ++baseId;
                ++assigned;
            }
        }
        
        return assigned;
    }
};

PYBIND11_MODULE(module, m) {
    m.doc() = "Python bindings for the psdparse library";

    py::class_<PythonPSD>(m, "PSD")
        .def(py::init<>())
        .def("load_from_file", &PythonPSD::loadFromFile, "Load PSD data from a file", py::arg("filepath"))
        .def("load_from_bytes", &PythonPSD::loadFromBytes, "Load PSD data from bytes", py::arg("bytes"))
        .def("get_basic_info", &PythonPSD::getBasicInfo, "Get basic information about the PSD file")
        .def("get_layer_type", &PythonPSD::getLayerType, "Get layer type", py::arg("layer_no"))
        .def("get_layer_name", &PythonPSD::getLayerName, "Get layer name", py::arg("layer_no"))
        .def("get_layer_info", &PythonPSD::getLayerInfo, "Get detailed layer information", py::arg("layer_no"))
        .def("get_layer_data", &PythonPSD::getLayerData, "Get layer image data as numpy array", 
             py::arg("layer_no"), py::arg("mode") = "maskedimage")
        .def("get_layer_data_raw", [](PythonPSD &self, int layerNo) { 
            return self.getLayerData(layerNo, "raw"); 
        }, "Get raw layer image data", py::arg("layer_no"))
        .def("get_layer_data_mask", [](PythonPSD &self, int layerNo) { 
            return self.getLayerData(layerNo, "mask"); 
        }, "Get layer mask data", py::arg("layer_no"))
        .def("get_blend", &PythonPSD::getBlend, "Get composite image")
        .def("get_slices", &PythonPSD::getSlices, "Get slice information")
        .def("get_guides", &PythonPSD::getGuides, "Get guide information")
        .def("get_layer_comp", &PythonPSD::getLayerComp, "Get layer composition information")
        .def("assign_auto_ids", &PythonPSD::assignAutoIds, "Assign auto IDs to layers", py::arg("base_id") = 0)
        .def_property_readonly("width", [](const PythonPSD &self) -> int { 
            return self.getBasicInfo().contains("width") ? self.getBasicInfo()["width"].cast<int>() : -1; 
        })
        .def_property_readonly("height", [](const PythonPSD &self) -> int { 
            return self.getBasicInfo().contains("height") ? self.getBasicInfo()["height"].cast<int>() : -1; 
        })
        .def_property_readonly("channels", [](const PythonPSD &self) -> int { 
            return self.getBasicInfo().contains("channels") ? self.getBasicInfo()["channels"].cast<int>() : -1; 
        })
        .def_property_readonly("depth", [](const PythonPSD &self) -> int { 
            return self.getBasicInfo().contains("depth") ? self.getBasicInfo()["depth"].cast<int>() : -1; 
        })
        .def_property_readonly("color_mode", [](const PythonPSD &self) -> int { 
            return self.getBasicInfo().contains("color_mode") ? self.getBasicInfo()["color_mode"].cast<int>() : -1; 
        })
        .def_property_readonly("layer_count", [](const PythonPSD &self) -> int { 
            return self.getBasicInfo().contains("layer_count") ? self.getBasicInfo()["layer_count"].cast<int>() : -1; 
        });

    // Color mode constants
    py::enum_<psd::ColorMode>(m, "ColorMode")
        .value("BITMAP", psd::COLOR_MODE_BITMAP)
        .value("GRAYSCALE", psd::COLOR_MODE_GRAYSCALE)
        .value("INDEXED", psd::COLOR_MODE_INDEXED)
        .value("RGB", psd::COLOR_MODE_RGB)
        .value("CMYK", psd::COLOR_MODE_CMYK)
        .value("MULTICHANNEL", psd::COLOR_MODE_MULTICHANNEL)
        .value("DUOTONE", psd::COLOR_MODE_DUOTONE)
        .value("LAB", psd::COLOR_MODE_LAB)
        .export_values();

    // Layer type constants
    py::enum_<psd::LayerType>(m, "LayerType")
        .value("NORMAL", psd::LAYER_TYPE_NORMAL)
        .value("FOLDER", psd::LAYER_TYPE_FOLDER)
        .value("HIDDEN", psd::LAYER_TYPE_HIDDEN)
        .export_values();

    // Blend mode constants
    py::enum_<psd::BlendMode>(m, "BlendMode")
        .value("NORMAL", psd::BLEND_MODE_NORMAL)
        .value("DARKEN", psd::BLEND_MODE_DARKEN)
        .value("MULTIPLY", psd::BLEND_MODE_MULTIPLY)
        .value("COLOR_BURN", psd::BLEND_MODE_COLOR_BURN)
        .value("LINEAR_BURN", psd::BLEND_MODE_LINEAR_BURN)
        .value("LIGHTEN", psd::BLEND_MODE_LIGHTEN)
        .value("SCREEN", psd::BLEND_MODE_SCREEN)
        .value("COLOR_DODGE", psd::BLEND_MODE_COLOR_DODGE)
        .value("LINEAR_DODGE", psd::BLEND_MODE_LINEAR_DODGE)
        .value("OVERLAY", psd::BLEND_MODE_OVERLAY)
        .value("SOFT_LIGHT", psd::BLEND_MODE_SOFT_LIGHT)
        .value("HARD_LIGHT", psd::BLEND_MODE_HARD_LIGHT)
        .value("VIVID_LIGHT", psd::BLEND_MODE_VIVID_LIGHT)
        .value("LINEAR_LIGHT", psd::BLEND_MODE_LINEAR_LIGHT)
        .value("PIN_LIGHT", psd::BLEND_MODE_PIN_LIGHT)
        .value("HARD_MIX", psd::BLEND_MODE_HARD_MIX)
        .value("DIFFERENCE", psd::BLEND_MODE_DIFFERENCE)
        .value("EXCLUSION", psd::BLEND_MODE_EXCLUSION)
        .value("DISSOLVE", psd::BLEND_MODE_DISSOLVE)
        .value("DARKER_COLOR", psd::BLEND_MODE_DARKER_COLOR)
        .value("LIGHTER_COLOR", psd::BLEND_MODE_LIGHTER_COLOR)
        .value("SUBTRACT", psd::BLEND_MODE_SUBTRACT)
        .value("DIVIDE", psd::BLEND_MODE_DIVIDE)
        .export_values();
}
