// --------------------------------------------------------------------------------------
// Die zielsuchende MiniRakete
//
// --------------------------------------------------------------------------------------

#include "Gegner_MiniRocket.hpp"
#include "stdafx.hpp"

// --------------------------------------------------------------------------------------
// Konstruktor
// --------------------------------------------------------------------------------------

GegnerMiniRocket::GegnerMiniRocket(int Wert1, int Wert2, bool Light) {
    Handlung = GEGNER::INIT;
    Energy = 20;
    ChangeLight = Light;
    Destroyable = true;
    Value1 = Wert1;
    Value2 = Wert2;
    SmokeDelay = 0.0f;
    FlareSin = 0.0f;
    OwnDraw = true;
    ySpeed = -20.0f;
    AnimCount = 3.0f;
    rot = 0.0f;
}

// --------------------------------------------------------------------------------------
// Eigene Draw Funktion
// --------------------------------------------------------------------------------------

void GegnerMiniRocket::DoDraw() {
    // Flare rendern
    //
    DirectGraphics.SetAdditiveMode();

    // DKS - Support new trig sin/cos lookup table and use deg/rad versions of sin/cos, optimized:
    // float xoff = static_cast<float>(sin((360 - rot) / 180.0f * PI) * 12.0f);
    // float yoff = static_cast<float>(cos((360 - rot) / 180.0f * PI) * 12.0f);
    // Projectiles.LavaFlare.RenderSpriteScaled (static_cast<float>(xPos-TileEngine.XOffset) - 15.0f -
    // static_cast<float>(sin(FlareSin) * 1.0f) + xoff,
    //                              static_cast<float>(yPos-TileEngine.YOffset) - 15.0f -
    //                              static_cast<float>(sin(FlareSin) * 1.0f) + yoff, static_cast<int>(40.0f +
    //                              (float)sin(FlareSin) * 2.0f), static_cast<int>(40.0f + (float)sin(FlareSin) * 2.0f),
    //                              0, 0xFFFF8822);
    float xoff = sin_deg(360.0f - rot) * 12.0f;
    float yoff = cos_deg(360.0f - rot) * 12.0f;
    float sin_FlareSin = sin_rad(FlareSin);
    Projectiles.LavaFlare.RenderSpriteScaled(xPos - TileEngine.XOffset - 15.0f - sin_FlareSin + xoff,
                                             yPos - TileEngine.YOffset - 15.0f - sin_FlareSin + yoff,
                                             static_cast<int>(40.0f + sin_FlareSin * 2.0f),
                                             static_cast<int>(40.0f + sin_FlareSin * 2.0f), 0, 0xFFFF8822);

    DirectGraphics.SetColorKeyMode();

    // Rakete rendern
    //
    pGegnerGrafix[GegnerArt]->RenderSpriteRotated(xPos - TileEngine.XOffset,
                                                  yPos - TileEngine.YOffset,
                                                  rot, 0, 0xFFFFFFFF);
}

// --------------------------------------------------------------------------------------
// "Bewegungs KI"
// --------------------------------------------------------------------------------------

void GegnerMiniRocket::DoKI() {
    BlickRichtung = DirectionEnum::LINKS;

    // Am Anfang erst etwas senkrecht hoch fliegen
    if (AnimCount > 0.0f)
        AnimCount -= Timer.sync(1.0f);

    // Flare pulsieren lassen
    //
    FlareSin += Timer.sync(3.0f);

    if (FlareSin > TWO_PI)
        FlareSin = 0;

    if (AnimCount <= 0.0f) {
        // Winkel zum Spieler ausrechnen
        //

        // Abstände berechnen
        float dx = (xPos + 6) - (pAim->xpos + 35);
        float dy = (yPos + 12) - (pAim->ypos + 20);

        // Division durch Null verhinden
        if (dy == 0.0f)
            dy = 0.01f;

        // DKS - converted to float, optimized:
        // w = static_cast<float>(atan(dx / dy) * 360.0f / (D3DX_PI * 2));
        float w = RadToDeg(atanf(dx / dy));

        if (dx >= 0.0f && dy >= 0.0f)
            rot = w;
        else if (dx > 0.0f && dy < 0.0f)
            rot = 180.0f + w;
        else if (dx < 0.0f && dy > 0.0f)
            rot = 360.0f + w;
        else if (dx < 0.0f && dy < 0.0f)
            rot = 180.0f + w;
        rot = 360.0f - rot;

        // Auf Spieler zubewegen
        //
        if (xPos + 6 < pAim->xpos + 35)
            xAcc = 2.5f;
        else
            xAcc = -2.5f;

        if (yPos + 12 < pAim->ypos + 40)
            yAcc = 2.5f;
        else
            yAcc = -2.5f;

        xSpeed = std::clamp(xSpeed, -10.0f, 10.0f);
        ySpeed = std::clamp(ySpeed, -10.0f, 10.0f);
    }

    // Rauchen lassen
    SmokeDelay -= Timer.sync(1.0f);

    while (SmokeDelay < 0.0f) {
        SmokeDelay += 0.5f;
        // DKS - Support new trig sin/cos lookup table and use deg/rad versions of sin/cos:
        // PartikelSystem.PushPartikel(xPos - 8 + static_cast<float>(sin((360 - rot) / 180.0f * PI) * 15.0f),
        //                              yPos - 8 + static_cast<float>(cos((360 - rot) / 180.0f * PI) * 15.0f), SMOKE);
        PartikelSystem.PushPartikel(xPos - 8.0f + (sin_deg(360.0f - rot) * 15.0f),
                                    yPos - 8.0f + (cos_deg(360.0f - rot) * 15.0f), SMOKE);
    }

    // Spieler getroffen?
    // Dann explodieren lassen und Energy abziehen
    //
    TestDamagePlayers(8.0f, true);

    // an der Wand explodieren
    if (blockl & BLOCKWERT_WAND || blockr & BLOCKWERT_WAND || blocko & BLOCKWERT_WAND || blocku & BLOCKWERT_WAND)
        Energy = 0.0f;
}

// --------------------------------------------------------------------------------------
// FetteRakete explodiert
// --------------------------------------------------------------------------------------

void GegnerMiniRocket::GegnerExplode() {
    SoundManager.PlayWave(100, 128, 8000 + random(4000), SOUND::EXPLOSION1);

    for (int i = 0; i < 5; i++)
        PartikelSystem.PushPartikel(xPos + static_cast<float>(random(20)),
                                    yPos - 10.0f + static_cast<float>(random(20)), SMOKE);

    for (int i = 0; i < 5; i++)
        PartikelSystem.PushPartikel(xPos + static_cast<float>(random(20)),
                                    yPos - 10.0f + static_cast<float>(random(20)), MINIFLARE);

    Player[0].Score += 50;
}
