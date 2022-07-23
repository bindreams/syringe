#define MINIAUDIO_IMPLEMENTATION
#include <cstdio>
#include <iostream>
#include <resources.hpp>
#include <span>
#include <stdexcept>
#include "miniaudio.h"

using namespace std;

/// Not important for this example.
void play_music_from_bytes(const uint8_t* data, size_t size) {
	auto data_callback = [](ma_device* pDevice, void* pOutput, const void*, ma_uint32 frameCount) {
		auto pDecoder = static_cast<ma_decoder*>(pDevice->pUserData);
		if (pDecoder == nullptr) return;

		ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, nullptr);
	};

	ma_decoder decoder;
	if (ma_decoder_init_memory(data, size, nullptr, &decoder) != MA_SUCCESS) {
		throw runtime_error("could not initialize decoder");
	}

	ma_device_config deviceConfig;
	deviceConfig = ma_device_config_init(ma_device_type_playback);
	deviceConfig.playback.format = decoder.outputFormat;
	deviceConfig.playback.channels = decoder.outputChannels;
	deviceConfig.sampleRate = decoder.outputSampleRate;
	deviceConfig.dataCallback = data_callback;
	deviceConfig.pUserData = &decoder;

	ma_device device;
	if (ma_device_init(nullptr, &deviceConfig, &device) != MA_SUCCESS) {
		ma_decoder_uninit(&decoder);
		throw runtime_error("could not initialize playback device");
	}

	if (ma_device_start(&device) != MA_SUCCESS) {
		ma_device_uninit(&device);
		ma_decoder_uninit(&decoder);
		throw runtime_error("could not start playback device");
	}

	cout << "Press any key to stop...\n";
	cin.get();
	cin.get();

	ma_device_uninit(&device);
	ma_decoder_uninit(&decoder);
}

/// Get all file names as an array at compile-time.
constexpr auto get_names() {
	constexpr size_t file_count = example::resources.size();
	std::array<std::string_view, file_count> names;

	int i = 0;
	for (auto& [name, data] : example::resources) {
		names[i++] = name;
	}

	return names;
}

int main() {
	constexpr auto names = get_names();

	while (true) {
		cout << "Select a composition of your choice:\n";

		for (int i = 0; i < names.size(); i++) {
			cout << "(" << (i + 1) << ") " << names[i] << "\n";
		}
		cout << "(0) Exit the music player\n> ";

		int choice;
		cin >> choice;

		if (choice == 0) return 0;
		choice--;

		span<const uint8_t> chosen_music = example::resources[names[choice]];
		play_music_from_bytes(chosen_music.data(), chosen_music.size());
	}
}
