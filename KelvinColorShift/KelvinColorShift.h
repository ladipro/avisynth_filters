class Helpers {
public:
  template<typename S, typename D>
  static D Clamp(S v) {
    if (v < std::numeric_limits<D>::min()) {
      return std::numeric_limits<D>::min();
    }
    if (v > std::numeric_limits<D>::max()) {
      return std::numeric_limits<D>::max();
    }
    return (D)v;
  }
};

// Represents a color as three 16-bit signed numbers and defines some
// intuitive as well as totally wacky operations on it.
struct RGB48 {
  short R;
  short G;
  short B;

  RGB48(short r, short g, short b) :
    R(r), G(g), B(b) {
  }

  RGB48() : RGB48(0, 0, 0) {
  }

  // From BGR
  RGB48(unsigned char* rgb8) :
    RGB48((short)rgb8[2] * 128, (short)rgb8[1] * 128, (short)rgb8[0] * 128) {
  }

  void ToRGB8(unsigned char* rgb8) {
    rgb8[0] = B8();
    rgb8[1] = G8();
    rgb8[2] = R8();
  }

  RGB48 operator+(const RGB48& rhs) const {
    return RGB48(
      Clamp((int)R + (int)rhs.R),
      Clamp((int)G + (int)rhs.G),
      Clamp((int)B + (int)rhs.B)
      );
  }

  RGB48 operator-(const RGB48& rhs) const {
    return RGB48(
      Clamp((int)R - (int)rhs.R),
      Clamp((int)G - (int)rhs.G),
      Clamp((int)B - (int)rhs.B)
      );
  }

  RGB48 operator+(int rhs) const {
    return RGB48(
      Clamp((int)R + rhs),
      Clamp((int)G + rhs),
      Clamp((int)B + rhs)
      );
  }

  RGB48 operator-(int rhs) const {
    return *this + (-rhs);
  }

  // This is the operation we apply to pixels in the video frame,
  // using the white balance shift as the right-hand-side argument.
  RGB48& operator*=(const RGB48& rhs) {
    int y = Y();
    *this = *this + RGB48(
      Clamp((y * rhs.R) / SHRT_MAX),
      Clamp((y * rhs.G) / SHRT_MAX),
      Clamp((y * rhs.B) / SHRT_MAX)
      );
    return *this;
  }

  // BT.2020
  short Y() const { return (short)(0.299 * R + 0.587 * G + 0.114 * B); }
  short U() const { return (short)(-0.168736 * R - 0.331264 * G + 0.5 * B); }
  short V() const { return (short)(0.5 * R - 0.418688 * G - 0.081312 * B); }

  unsigned char R8() const { return (R > 0) ? (R / 128) : 0; }
  unsigned char G8() const { return (G > 0) ? (G / 128) : 0; }
  unsigned char B8() const { return (B > 0) ? (B / 128) : 0; }

private:
  static short Clamp(int v) {
    return Helpers::Clamp<int, short>(v);
  }
};
