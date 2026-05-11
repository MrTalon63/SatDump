#include "core/plugin.h"
#include "logger.h"
#include "pipeline/module.h"
#include "common/net/udp.h"
#include "common/widgets/themed_widgets.h"
#include <cmath>
#include <memory>
#include <atomic>
#include <chrono>
#include <thread>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include "imgui/imgui.h"

namespace satdump {
    class UDPFrameSenderModule : public pipeline::ProcessingModule {
    private:
        std::string d_ip;
        int d_port;
        int d_frame_bytes;
        std::shared_ptr<net::UDPClient> udp_client;
        std::vector<uint8_t> buffer;
        std::atomic<uint64_t> frames_sent{0};
        std::atomic<uint64_t> bytes_sent{0};
        
        std::chrono::time_point<std::chrono::steady_clock> last_calc_time;
        uint64_t last_bytes_sent = 0;
        float speed_history[100] = {0};
        uint64_t byte_history[10] = {0};

    public:
        UDPFrameSenderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : ProcessingModule(input_file, output_file_hint, parameters) {
            
            d_ip = parameters.count("ip") ? parameters["ip"].get<std::string>() : "127.0.0.1";
            d_port = parameters.count("port") ? parameters["port"].get<int>() : 9000;
            
            if (parameters.count("cadu_size")) {
                d_frame_bytes = std::ceil(parameters["cadu_size"].get<int>() / 8.0);
            } else if (parameters.count("frame_size")) {
                d_frame_bytes = parameters["frame_size"].get<int>();
            } else {
                d_frame_bytes = 1024;
                logger->warn("UDPFrameSenderModule: No cadu_size or frame_size provided by pipeline, defaulting to 1024 bytes.");
            }

            buffer.resize(d_frame_bytes);
            
            try {
                udp_client = std::make_shared<net::UDPClient>((char*)d_ip.c_str(), d_port);
            } catch (std::exception& e) {
                logger->error("UDPFrameSenderModule: Failed to create UDP client: %s", e.what());
            }
            
            last_calc_time = std::chrono::steady_clock::now();
        }

        ~UDPFrameSenderModule() = default;

        std::vector<pipeline::ModuleDataType> getInputTypes() override {
            return {pipeline::DATA_STREAM, pipeline::DATA_FILE};
        }

        std::vector<pipeline::ModuleDataType> getOutputTypes() override {
            return {pipeline::DATA_STREAM, pipeline::DATA_FILE};
        }

        void drawUI(bool window) override {
            ImGui::Begin("UDP Frame Sender", NULL, window ? 0 : NOWINDOW_FLAGS);

            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<float> elapsed_recent = now - last_calc_time;

            uint64_t current_bytes = bytes_sent.load();

            if (elapsed_recent.count() >= 0.1f) {
                uint64_t diff = current_bytes - last_bytes_sent;
                
                std::memmove(&byte_history[0], &byte_history[1], (10 - 1) * sizeof(uint64_t));
                byte_history[10 - 1] = diff;

                uint64_t total_bytes_1s = 0;
                for (int i = 0; i < 10; i++) {
                    total_bytes_1s += byte_history[i];
                }

                float mbps = (total_bytes_1s * 8.0f) / 1000000.0f;
                
                std::memmove(&speed_history[0], &speed_history[1], (100 - 1) * sizeof(float));
                speed_history[100 - 1] = mbps;

                last_bytes_sent = current_bytes;
                last_calc_time = now;
            }

            float short_avg_mbps = speed_history[99];

            ImGui::Text("Destination: %s:%d", d_ip.c_str(), d_port);
            ImGui::Text("Frame Size: %d bytes", d_frame_bytes);
            ImGui::Text("Frames Sent: %llu", (unsigned long long)frames_sent.load());
            ImGui::Text("Data Sent: %.2f MB", (double)current_bytes / 1048576.0);

            if (short_avg_mbps >= 1.0f) {
                ImGui::Text("Speed: %.3f Mbps", short_avg_mbps);
            } else {
                ImGui::Text("Speed: %.3f kbps", short_avg_mbps * 1000.0f);
            }

            float max_speed = 0.01f;
            for (int i = 0; i < 100; i++) {
                if (speed_history[i] > max_speed) max_speed = speed_history[i];
            }
            
            char overlay[32];
            if (max_speed >= 1.0f) {
                snprintf(overlay, sizeof(overlay), "Max: %.2f Mbps", max_speed);
            } else {
                snprintf(overlay, sizeof(overlay), "Max: %.2f kbps", max_speed * 1000.0f);
            }

            float plot_width = std::min(ImGui::GetContentRegionAvail().x, 200.0f);
            satdump::widgets::ThemedPlotLines(style::theme.plot_bg.Value, "##speedplot", speed_history, 100, 0, overlay, 0.0f, max_speed * 1.2f, ImVec2(plot_width, 80));

            ImGui::End();
        }

        void process() override {
            logger->info("UDPFrameSenderModule: Sending frames to %s:%d (Size: %d bytes)", d_ip.c_str(), d_port, d_frame_bytes);

            std::ofstream outfile;
            if (output_data_type == pipeline::DATA_FILE) {
                d_output_file = d_output_file_hint + ".frm";
                outfile.open(d_output_file, std::ios::binary);
            }

            if (input_data_type == pipeline::DATA_FILE) {
                std::ifstream infile(d_input_file, std::ios::binary);

                while (infile && input_active.load()) {
                    infile.read((char*)buffer.data(), d_frame_bytes);
                    int read_size = infile.gcount();
                    
                    if (read_size > 0) {
                        if (read_size == d_frame_bytes && udp_client) {
                            try {
                                udp_client->send(buffer.data(), read_size);
                                frames_sent++;
                                bytes_sent += read_size;
                                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                            } catch (std::exception& e) {}
                        }
                        if (outfile.is_open()) {
                            outfile.write((char*)buffer.data(), read_size);
                        }
                    } else {
                        break;
                    }
                }
            } else {
                while (input_active.load()) {
                    if (input_fifo == nullptr) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        continue;
                    }

                    int total_read = 0;
                    while (total_read < d_frame_bytes && input_active.load()) {
                        int read_size = input_fifo->read(buffer.data() + total_read, d_frame_bytes - total_read);
                        if (read_size > 0) {
                            total_read += read_size;
                        } else {
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        }
                    }

                    if (total_read > 0) {
                        if (total_read == d_frame_bytes && udp_client) {
                            try {
                                udp_client->send(buffer.data(), total_read);
                                frames_sent++;
                                bytes_sent += total_read;
                            } catch (std::exception& e) {}
                        }
                        
                        if (output_data_type == pipeline::DATA_STREAM && output_fifo != nullptr) {
                            output_fifo->write(buffer.data(), total_read);
                        } else if (output_data_type == pipeline::DATA_FILE && outfile.is_open()) {
                            outfile.write((char*)buffer.data(), total_read);
                        }
                    }
                }
            }
        }

        static std::string getID() { return "udp_frame_sender"; }
        std::string getIDM() override { return getID(); }
        static nlohmann::json getParams() { return {}; }
        static std::shared_ptr<pipeline::ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters) {
            return std::make_shared<UDPFrameSenderModule>(input_file, output_file_hint, parameters);
        }
    };

    class UDPFrameSenderPlugin : public Plugin {
    public:
        std::string getID() override {
            return "udp_frame_sender";
        }

        void init() override {
            logger->info("Loaded UDP Frame Sender plugin!");
            satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler);
        }

        static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) {
            REGISTER_MODULE_EXTERNAL(evt.modules_registry, UDPFrameSenderModule);
        }
    };
}

PLUGIN_LOADER(satdump::UDPFrameSenderPlugin)