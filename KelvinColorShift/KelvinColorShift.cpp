// KelvinColorShift.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "KelvinColorShift.h"

class KelvinColorShift : public GenericVideoFilter {
  RGB48 rgb_shift;

public:
  KelvinColorShift(PClip _child, int from_temp, int to_temp, IScriptEnvironment* env)
    : GenericVideoFilter(_child) {
    if (from_temp < 1000 || from_temp > 10000 ||
        to_temp < 1000 || to_temp > 10000) {
      env->ThrowError("KelvinColorShift: Color temperature must be between 1000 and 10000!");
    }
    if (!vi.IsRGB() && !(vi.IsPlanar() && vi.IsYUV())) {
      env->ThrowError("KelvinColorShift: Unsupported color format. RGB or planar YUV data only!");
    }

    RGB48 old_wb = ComputeWhiteBalance(from_temp);
    RGB48 new_wb = ComputeWhiteBalance(to_temp);
    rgb_shift = old_wb - new_wb;

    // normalize the shift to preserve luminosity
    rgb_shift = rgb_shift - rgb_shift.Y();
  }

  // Based on http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/
  RGB48 ComputeWhiteBalance(int temp) {
    RGB48 white_balance;
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

    if (vi.IsRGB()) {
      unsigned char* ptr = frame->GetWritePtr();
      int pitch = frame->GetPitch();
      int row_size = frame->GetRowSize();
      int height = frame->GetHeight();
      int bytes_per_pixel = vi.IsRGB24() ? 3 : 4;

      for (int y = 0; y < height; y++) {
        for (int x = 0; x < row_size; x += bytes_per_pixel) {
          RGB48 rgb(&ptr[x]);
          rgb *= rgb_shift;
          rgb.ToRGB8(&ptr[x]);
        }
        ptr += pitch;
      }
    } else {
      _ASSERT(vi.IsPlanar() && vi.IsYUV());
      int planes[] = {
        PLANAR_U,
        PLANAR_V
      };
      char plane_shifts[] = {
        (char)(rgb_shift.U() >> 8),
        (char)(rgb_shift.V() >> 8)
      };
      C_ASSERT(_countof(planes) == _countof(plane_shifts));

      for (int p = 0; p < _countof(planes); p++) {
        unsigned char* ptr = frame->GetWritePtr(planes[p]);
        int pitch = frame->GetPitch(planes[p]);
        int row_size = frame->GetRowSize(planes[p]);
        int height = frame->GetHeight(planes[p]);

        for (int y = 0; y < height; y++) {
          for (int x = 0; x < row_size; x++) {
            ptr[x] = Helpers::Clamp<short, unsigned char>((short)ptr[x] + plane_shifts[p]);
          }
          ptr += pitch;
        }
      }
    }

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
