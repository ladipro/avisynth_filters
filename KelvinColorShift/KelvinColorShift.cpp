// KelvinColorShift.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "KelvinColorShift.h"

class KelvinColorShift : public GenericVideoFilter {
  RGB16 rgb_shift;

public:
  KelvinColorShift(PClip _child, int from_temp, int to_temp, IScriptEnvironment* env)
    : GenericVideoFilter(_child) {
    if (from_temp < 1000 || from_temp > 10000 ||
        to_temp < 1000 || to_temp > 10000) {
      env->ThrowError("KelvinColorShift: Color temperature must be between 1000 and 10000!");
    }

    RGB16 old_wb = ComputeWhiteBalance(from_temp);
    RGB16 new_wb = ComputeWhiteBalance(to_temp);

    rgb_shift.R = old_wb.R - new_wb.R;
    rgb_shift.G = old_wb.G - new_wb.G;
    rgb_shift.B = old_wb.B - new_wb.B;

    // normalize the shift to preserve luminosity
    rgb_shift = rgb_shift - rgb_shift.Y();
  }

  // Based on http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/
  RGB16 ComputeWhiteBalance(int temp) {
    RGB16 white_balance;
    double temp_fp = (double)temp / 100;

    // red
    if (temp <= 6680) {
      white_balance.R = SHRT_MAX;
    }
    else {
      double r_fp = 329.698727446 * pow(temp_fp - 60, -0.1332047592);
      white_balance.R = (short)(128 * r_fp);
    }

    // green
    double g_fp;
    if (temp <= 6600) {
      g_fp = 99.4708025861 * log(temp_fp) - 161.1195681661;
    }
    else {
      g_fp = 288.1221695283 * pow(temp_fp - 60, -0.0755148492);
    }
    white_balance.G = (short)(128 * g_fp);

    // blue
    if (temp >= 6540) {
      white_balance.B = SHRT_MAX;
    }
    else if (temp <= 1900) {
      white_balance.B = 0;
    }
    else {
      double b_fp = 138.5177312231 * log(temp_fp - 10) - 305.0447927307;
      white_balance.B = (short)(128 * b_fp);
    }

    return white_balance;
  }

  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) {
    PVideoFrame frame = child->GetFrame(n, env);
    env->MakeWritable(&frame);

    unsigned char* ptr = frame->GetWritePtr();
    int pitch = frame->GetPitch();

    int bytes_per_pixel = vi.IsRGB24() ? 3 : 4;
#define FRAME_XY_TO_INDEX(x, y) ((y) * pitch) + ((x) * bytes_per_pixel)

    // TODO: Beautify, support YUV color spaces?
    for (int y = 0; y < frame->GetHeight(); y++) {
      for (int x = 0; x < frame->GetRowSize() / bytes_per_pixel; x++) {
        size_t index = FRAME_XY_TO_INDEX(x, y);

        RGB16 rgb(&ptr[index]);
        rgb = rgb * rgb_shift;
        ptr[index] = rgb.B8();
        ptr[index + 1] = rgb.G8();
        ptr[index + 2] = rgb.R8();
      }
    }

#undef FRAME_XY_TO_INDEX
    return frame;
  }
};

AVSValue __cdecl Create_KelvinColorShift(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return new KelvinColorShift(args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), env);
}

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env) {
  env->AddFunction("KelvinColorShift", "c[from_temp]i[to_temp]i", Create_KelvinColorShift, 0);
  return "Kelvin color shifter plugin";
}