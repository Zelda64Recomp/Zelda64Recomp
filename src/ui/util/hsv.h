#ifndef RECOMP_UI_HSV
#define RECOMP_UI_HSV


namespace recompui {
    typedef struct RgbColor
    {
        union {
            struct {
                unsigned char r;
                unsigned char g;
                unsigned char b;
            };
            unsigned char data[3]; // Array access
        };

        // Operator[] to access members by index
        unsigned char& operator[](int index) {
            return data[index];
        }

        // Const version for read-only access
        const unsigned char& operator[](int index) const {
            return data[index];
        }
    } RgbColor;

    typedef struct HsvColor
    {
        union {
            struct {
                unsigned char h;
                unsigned char s;
                unsigned char v;
            };
            unsigned char data[3]; // Array access
        };

        // Operator[] to access members by index
        unsigned char& operator[](int index) {
            return data[index];
        }

        // Const version for read-only access
        const unsigned char& operator[](int index) const {
            return data[index];
        }
    } HsvColor;

    typedef struct HsvColorF
    {
        union {
            struct {
                float h; // 0-360
                float s; // 0-1
                float v; // 0-1
            };
            float data[3]; // Array access
        };

        // Operator[] to access members by index
        float& operator[](int index) {
            return data[index];
        }

        // Const version for read-only access
        const float& operator[](int index) const {
            return data[index];
        }
    } HsvColorF;

    void HsvToRgb(HsvColor& hsv, RgbColor& rgb);
    void HsvFToRgb(HsvColorF in, RgbColor& out);
    void RgbToHsv(RgbColor& rgb, HsvColor& hsv);
}

#endif
