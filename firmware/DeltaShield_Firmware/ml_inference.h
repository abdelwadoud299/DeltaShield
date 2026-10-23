#ifndef ML_INFERENCE_H
#define ML_INFERENCE_H

#include <DeltaShield_inferencing.h>
#include "esp_camera.h"

static camera_fb_t *_ei_fb = nullptr;

static int ei_get_data(size_t offset, size_t length, float *out) {
    if (_ei_fb == nullptr) return -1;

    const uint8_t *buf = _ei_fb->buf;
    for (size_t i = 0; i < length; i++) {
        size_t px  = (offset + i) * 2;        // RGB565: 2 bytes per pixel
        uint8_t byte1 = buf[px];              // High byte: RRRRRGGG
        uint8_t byte2 = buf[px + 1];          // Low byte:  GGGBBBBB
        
        uint8_t r  = (byte1 & 0xF8);
        uint8_t g  = ((byte1 & 0x07) << 5) | ((byte2 & 0xE0) >> 3);
        uint8_t b  = ((byte2 & 0x1F) << 3);
        
        out[i]     = (float)((r << 16) | (g << 8) | b);
    }
    return 0;
}

float getWeedConfidence() {
    _ei_fb = esp_camera_fb_get();
    if (_ei_fb == nullptr) {
        Serial.println("[ML] Camera frame capture failed");
        return 0.0f;
    }

    if (_ei_fb->width  != EI_CLASSIFIER_INPUT_WIDTH  ||
        _ei_fb->height != EI_CLASSIFIER_INPUT_HEIGHT) {
        Serial.printf("[ML] Frame size mismatch: got %dx%d, model wants %dx%d\n",
                      _ei_fb->width, _ei_fb->height,
                      EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT);
        esp_camera_fb_return(_ei_fb);
        _ei_fb = nullptr;
        return 0.0f;
    }

    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data     = &ei_get_data;

    ei_impulse_result_t result = {0};
    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);

    esp_camera_fb_return(_ei_fb);
    _ei_fb = nullptr;

    if (err != EI_IMPULSE_OK) {
        Serial.printf("[ML] run_classifier error: %d\n", err);
        return 0.0f;
    }

    float maxConf = 0.0f;
    for (uint16_t i = 0; i < result.bounding_boxes_count; i++) {
        auto &bb = result.bounding_boxes[i];
        if (bb.value > maxConf &&
            strcmp(bb.label, "weed") == 0) {
            maxConf = bb.value;
        }
    }
    return maxConf;
}

#endif
