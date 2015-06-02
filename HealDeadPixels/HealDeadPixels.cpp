// DeadPixelFilter.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "HealDeadPixels.h"

HealDeadPixels::HealDeadPixels(PClip _child, const char* mask_file, IScriptEnvironment* env)
  : GenericVideoFilter(_child) {
  if (!vi.IsRGB()) {
    env->ThrowError("HealDeadPixels: Unsupported color format. RGB data only!");
  }

  size_t len = strlen(mask_file);
  std::wstring mask_file_w(len, 0);
  std::use_facet<std::ctype<wchar_t> >(std::locale()).widen
    (&mask_file[0], &mask_file[0] + len, &mask_file_w[0]);

  std::unique_ptr<Gdiplus::Bitmap> bitmap(new Gdiplus::Bitmap(mask_file_w.c_str()));
  if (bitmap->GetWidth() != vi.width || bitmap->GetHeight() != vi.height) {
    env->ThrowError("HealDeadPixels: Mask bitmap does not match frame size!");
  }

  GeneratePixelHealRecipes(bitmap);
}

void HealDeadPixels::GeneratePixelHealRecipes(std::unique_ptr<Gdiplus::Bitmap> &bitmap) {
  int bitmap_width = bitmap->GetWidth();
  int bitmap_height = bitmap->GetHeight();

  for (int y = 0; y < bitmap_height; y++) {
    for (int x = 0; x < bitmap_width; x++) {
      if (IsDeadPixel(bitmap, x, y)) {
        // dead pixel, create its heal recipe
        PixelHealRecipe recipe(x, y);

        // first pass, find replacement pixels
        int idx = 0;
        for (int distance = 1;
             distance <= MAX_REPLACEMENT_DISTANCE && idx < MAX_REPLACEMENT_PIXELS;
             distance++) {
          for (int i = 0; i < 4 * distance; i++) {
            int offset = i / 4;
            int x_factor = (i & 1) ? 1 : -1;
            int y_factor = (i & 2) ? 1 : -1;
            recipe.replacements[idx].offset_x = x_factor * offset;
            recipe.replacements[idx].offset_y = y_factor * (distance - offset);

            int bitmap_x = recipe.frame_x + recipe.replacements[idx].offset_x;
            int bitmap_y = recipe.frame_y + recipe.replacements[idx].offset_y;
            if (!IsDeadPixel(bitmap, bitmap_x, bitmap_y)) {
              // this is a usable replacement, move to next index
              if (++idx >= MAX_REPLACEMENT_PIXELS) {
                break;
              }
              recipe.replacements[idx] = recipe.replacements[idx - 1];
            }
          }
        }

        // second pass, compute weights
        int distance_sum = 0;
        for (int i = 0; i < idx; i++) {
          // TODO: eliminate floating point arithmetics
          double distance = sqrt(
            recipe.replacements[i].offset_x * recipe.replacements[i].offset_x +
            recipe.replacements[i].offset_y * recipe.replacements[i].offset_y);
          distance = UINT16_MAX / pow(M_E, distance);
          recipe.replacements[i].weight = (int)distance;
          distance_sum += (int)distance;
        }
        for (int i = 0; i < idx; i++) {
          recipe.replacements[i].weight =
            (UINT16_MAX * (int)recipe.replacements[i].weight) / distance_sum;
        }
        for (int i = idx; i < MAX_REPLACEMENT_PIXELS; i++) {
          recipe.replacements[i].offset_x = 0;
          recipe.replacements[i].offset_y = 0;
          recipe.replacements[i].weight = 0;
        }
        pixel_recipes.push_back(std::move(recipe));
      }
    }
  }
}

bool HealDeadPixels::IsDeadPixel(
  std::unique_ptr<Gdiplus::Bitmap> &bitmap,
  int x,
  int y
  ) {
  if (x < 0 || (UINT)x >= bitmap->GetWidth() ||
      y < 0 || (UINT)y >= bitmap->GetHeight()) {
    // pixel outside of the frame is dead by default
    return true;
  }
  Gdiplus::Color color;
  bitmap->GetPixel(x, bitmap->GetHeight() - y - 1, &color);
  return (color.GetR() >= 128 || color.GetG() >= 128 || color.GetB() >= 128);
}

PVideoFrame __stdcall HealDeadPixels::GetFrame(int n, IScriptEnvironment* env) {

  PVideoFrame frame = child->GetFrame(n, env);
  env->MakeWritable(&frame);

  unsigned char* ptr = frame->GetWritePtr();
  int pitch = frame->GetPitch();

  int bytes_per_pixel = vi.IsRGB24() ? 3 : 4;
#define FRAME_XY_TO_INDEX(x, y) ((y) * pitch) + ((x) * bytes_per_pixel)

  // iterate over the recipes and fix all dead pixels one by one - done with
  // integer calculations only
  for (const auto& recipe : pixel_recipes) {
    int avg_r = 0, avg_g = 0, avg_b = 0;
    for (int i = 0; i < MAX_REPLACEMENT_PIXELS; i++) {
      if (recipe.replacements[i].weight > 0) {
        size_t index = FRAME_XY_TO_INDEX(
          recipe.frame_x + recipe.replacements[i].offset_x,
          recipe.frame_y + recipe.replacements[i].offset_y);
        avg_b += (int)recipe.replacements[i].weight * ptr[index];
        avg_g += (int)recipe.replacements[i].weight * ptr[index + 1];
        avg_r += (int)recipe.replacements[i].weight * ptr[index + 2];
      }
    }
    size_t index = FRAME_XY_TO_INDEX(
      recipe.frame_x,
      recipe.frame_y);
    ptr[index] = avg_b / UINT16_MAX;
    ptr[index + 1] = avg_g / UINT16_MAX;
    ptr[index + 2] = avg_r / UINT16_MAX;
  }

#undef FRAME_XY_TO_INDEX
  return frame;
}

AVSValue __cdecl Create_HealDeadPixels(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return new HealDeadPixels(args[0].AsClip(), args[1].AsString(""), env);
}

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env) {
  env->AddFunction("HealDeadPixels", "c[mask_image]s", Create_HealDeadPixels, 0);
  return "Dead pixel removal plugin";
}
