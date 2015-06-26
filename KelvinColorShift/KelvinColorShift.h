// Represents a color as three 16-bit signed numbers and defines some
// intuitive as well as totally wacky operations on it.
struct RGB16 {
  short R;
  short G;
  short B;

  RGB16(short r, short g, short b) :
    R(r), G(g), B(b) {
  }

  RGB16() : RGB16(0, 0, 0) {
  }

  // From BGR
  RGB16(unsigned char* rgb8) :
    RGB16((short)rgb8[2] * 128, (short)rgb8[1] * 128, (short)rgb8[0] * 128) {
  }

  RGB16 operator+(const RGB16& rhs) const {
    return RGB16(
      Clamp((int)R + (int)rhs.R),
      Clamp((int)G + (int)rhs.G),
      Clamp((int)B + (int)rhs.B)
      );
  }

  RGB16 operator-(const RGB16& rhs) const {
    return RGB16(
      Clamp((int)R - (int)rhs.R),
      Clamp((int)G - (int)rhs.G),
      Clamp((int)B - (int)rhs.B)
      );
  }

  RGB16 operator+(int rhs) const {
    return RGB16(
      Clamp((int)R + rhs),
      Clamp((int)G + rhs),
      Clamp((int)B + rhs)
      );
  }

  RGB16 operator-(int rhs) const {
    return *this + (-rhs);
  }

  // This is the operation we apply to pixels in the video frame,
  // using the white balance shift as the right-hand-side argument.
  RGB16 operator*(const RGB16& rhs) const {
    int y = Y();
    return *this + RGB16(
      Clamp((y * rhs.R) / SHRT_MAX),
      Clamp((y * rhs.G) / SHRT_MAX),
      Clamp((y * rhs.B) / SHRT_MAX)
      );
  }

  short Y() const {
    // Y = 0.299 R + 0.587 G + 0.114 B (dogscience)
    return (short)(0.299 * R + 0.587 * G + 0.114 * B);
  }

  unsigned char R8() const { return (R > 0) ? (R / 128) : 0; }
  unsigned char G8() const { return (G > 0) ? (G / 128) : 0; }
  unsigned char B8() const { return (B > 0) ? (B / 128) : 0; }

private:
  static short Clamp(int v) {
    if (v < SHRT_MIN) {
      return SHRT_MIN;
    }
    if (v > SHRT_MAX) {
      return SHRT_MAX;
    }
    return (short)v;
  }
};
