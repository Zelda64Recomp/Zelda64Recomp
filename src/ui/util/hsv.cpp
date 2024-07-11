#include "hsv.h"
#include <algorithm> // std::min, std::max and std::clamp
#include <math.h>

namespace recompui {

void HsvToRgb(HsvColor& hsv, RgbColor& rgb)
{
    unsigned char region, remainder, p, q, t;
    
    if (hsv.s == 0)
    {
        rgb.r = hsv.v;
        rgb.g = hsv.v;
        rgb.b = hsv.v;
        return;
    }
    
    region = hsv.h / 43;
    remainder = (hsv.h - (region * 43)) * 6; 
    
    p = (hsv.v * (255 - hsv.s)) >> 8;
    q = (hsv.v * (255 - ((hsv.s * remainder) >> 8))) >> 8;
    t = (hsv.v * (255 - ((hsv.s * (255 - remainder)) >> 8))) >> 8;
    
    switch (region)
    {
        case 0:
            rgb.r = hsv.v; rgb.g = t; rgb.b = p;
            break;
        case 1:
            rgb.r = q; rgb.g = hsv.v; rgb.b = p;
            break;
        case 2:
            rgb.r = p; rgb.g = hsv.v; rgb.b = t;
            break;
        case 3:
            rgb.r = p; rgb.g = q; rgb.b = hsv.v;
            break;
        case 4:
            rgb.r = t; rgb.g = p; rgb.b = hsv.v;
            break;
        default:
            rgb.r = hsv.v; rgb.g = p; rgb.b = q;
            break;
    }
}

void RgbToHsv(RgbColor& rgb, HsvColor& hsv)
{
    unsigned char rgbMin, rgbMax;

    rgbMin = rgb.r < rgb.g ? (rgb.r < rgb.b ? rgb.r : rgb.b) : (rgb.g < rgb.b ? rgb.g : rgb.b);
    rgbMax = rgb.r > rgb.g ? (rgb.r > rgb.b ? rgb.r : rgb.b) : (rgb.g > rgb.b ? rgb.g : rgb.b);
    
    hsv.v = rgbMax;
    if (hsv.v == 0)
    {
        hsv.h = 0;
        hsv.s = 0;
        return;
    }

    hsv.s = 255 * long(rgbMax - rgbMin) / hsv.v;
    if (hsv.s == 0)
    {
        hsv.h = 0;
        return;
    }

    if (rgbMax == rgb.r)
        hsv.h = 0 + 43 * (rgb.g - rgb.b) / (rgbMax - rgbMin);
    else if (rgbMax == rgb.g)
        hsv.h = 85 + 43 * (rgb.b - rgb.r) / (rgbMax - rgbMin);
    else
        hsv.h = 171 + 43 * (rgb.r - rgb.g) / (rgbMax - rgbMin);
}

static unsigned char clamp_255(float f) {
    return std::clamp((int)round(f), 0, 255);
}

void HsvFToRgb(HsvColorF in, RgbColor& out)
{
    float      hh, p, q, t, ff;
    long        i;

    unsigned char val = clamp_255(in.v * 255.0f);

    if (in.s <= 0.0) {
        out.r = val;
        out.g = val;
        out.b = val;
        return;
    }

    hh = in.h;
    if (hh >= 360.0f) hh = 0.0f;
    hh /= 60.0f;
    i = (float)hh;
    ff = hh - i;
    p = in.v * (1.0f - in.s);
    q = in.v * (1.0f - (in.s * ff));
    t = in.v * (1.0f - (in.s * (1.0f - ff)));

    unsigned char up = clamp_255(p * 255.0f);
    unsigned char uq = clamp_255(q * 255.0f);
    unsigned char ut = clamp_255(t * 255.0f);

    switch (i) {
    case 0:
        out.r = val;
        out.g = ut;
        out.b = up;
        return;
    case 1:
        out.r = uq;
        out.g = val;
        out.b = up;
        return;
    case 2:
        out.r = up;
        out.g = val;
        out.b = ut;
        return;
    case 3:
        out.r = up;
        out.g = uq;
        out.b = val;
        return;
    case 4:
        out.r = ut;
        out.g = up;
        out.b = val;
        return;
    case 5:
    default:
        out.r = val;
        out.g = up;
        out.b = uq;
        return;
    }
}

}
