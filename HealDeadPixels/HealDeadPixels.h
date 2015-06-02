// Maximum number of neighboring pixels whose values will be used to fix a dead one.
#define MAX_REPLACEMENT_PIXELS 24

// Maximum distance (in x+y) of neighboring pixels whose values will be used to fix a dead one.
#define MAX_REPLACEMENT_DISTANCE 10

// Describes one dead pixel
struct PixelHealRecipe {
  PixelHealRecipe(int x, int y)
    : frame_x(x), frame_y(y) {
  }
  int frame_x;
  int frame_y;
  struct {
    int8_t offset_x;
    int8_t offset_y;
    uint16_t weight; // 0 = 0%, UINT16_MAX = 100%
  } replacements[MAX_REPLACEMENT_PIXELS];
};

class HealDeadPixels : public GenericVideoFilter {
  std::vector<PixelHealRecipe> pixel_recipes;

public:
  HealDeadPixels(PClip _child, const char* mask_file, IScriptEnvironment* env);

  void GeneratePixelHealRecipes(std::unique_ptr<Gdiplus::Bitmap> &bitmap);
  static bool IsDeadPixel(
    std::unique_ptr<Gdiplus::Bitmap> &bitmap,
    int x,
    int y
  );

  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};
