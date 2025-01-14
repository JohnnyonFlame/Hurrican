// --------------------------------------------------------------------------------------
// Die Zitrone (Zieh Drone)
//
// Zieht dem Hurrican hinterher und ballert
// --------------------------------------------------------------------------------------

#include "Gegner_Zitrone.hpp"
#include "stdafx.hpp"

// --------------------------------------------------------------------------------------
// Konstruktor
// --------------------------------------------------------------------------------------

GegnerZitrone::GegnerZitrone(int Wert1, int Wert2, bool Light) {
    Handlung = GEGNER::STEHEN;
    Energy = 200;
    ChangeLight = Light;
    Destroyable = true;
    SmokeDelay = 1.0f;
    WackelOffset = 0.0f;
    OwnDraw = true;
    KnarreWinkel = 0.0f;
    ShotDelay = 0.0f;
    AlreadyDrawn = false;

    FollowCount = 0.0f;
}

// --------------------------------------------------------------------------------------
// Knarre zeigt auf Hurrican
// --------------------------------------------------------------------------------------

void GegnerZitrone::CalcKnarreWinkel() {

    float ydiv = (pAim->ypos + 40) - (yPos + 40);

    if (ydiv == 0.0f)
        ydiv = 0.00001f;

    float xdiv = (pAim->xpos + 35.0f) - (xPos + 60.0f);

    // DKS-converting to float, new rad/deg macros:
    // newwinkel = (float)atan(xdiv / ydiv) * 180.0f / D3DX_PI + 180.0f;
    float newwinkel = RadToDeg(atanf(xdiv / ydiv)) + 180.0f;

    if (xdiv >= 0.0f && ydiv >= 0.0f)
        newwinkel = newwinkel;
    else if (xdiv > 0.0f && ydiv < 0.0f)
        newwinkel = 180.0f + newwinkel;
    else if (xdiv < 0.0f && ydiv > 0.0f)
        newwinkel = 360.0f + newwinkel;
    else if (xdiv < 0.0f && ydiv < 0.0f)
        newwinkel = 180.0f + newwinkel;

    // Winkel begrenzen
    clampAngle(newwinkel);

    newwinkel = std::clamp(newwinkel, 100.0f, 260.0f);

    if (KnarreWinkel < newwinkel)
        KnarreWinkel += Timer.sync(20.0f);
    if (KnarreWinkel > newwinkel)
        KnarreWinkel -= Timer.sync(20.0f);
}

// --------------------------------------------------------------------------------------
// Rendern
//
// Flares links und rechts am Antrieb werden je nach Animationsphase
// vor oder hinter dem Gegner gerendert
//
// --------------------------------------------------------------------------------------

void GegnerZitrone::DoDraw() {
    int a = std::clamp(AnimPhase, 3, 7);

    float yoff = sin(WackelOffset) * 10.0f;

    if (AlreadyDrawn == false) {
        DirectGraphics.SetAdditiveMode();
        if (AnimPhase >= 5)
            Projectiles.LavaFlare.RenderSprite(-TileEngine.XOffset + xPos - 60.0f + static_cast<float>(a * 4),
                                               -TileEngine.YOffset + yPos - 10.0f + yoff, 0, 0xBBFFAA66);

        if (AnimPhase <= 5)
            Projectiles.LavaFlare.RenderSprite(-TileEngine.XOffset + xPos + 40.0f + static_cast<float>((a - 5) * 4),
                                               -TileEngine.YOffset + yPos - 10.0f + yoff, 0, 0xBBFFAA66);
        DirectGraphics.SetColorKeyMode();
    }

    // Knarre
    Gegner.DroneGun.RenderSpriteRotatedOffset(-TileEngine.XOffset + xPos + 73.0f - static_cast<float>(a * 5),
                                              -TileEngine.YOffset + yPos + 48.0f + yoff, KnarreWinkel, 0, 0,
                                              0xFFFFFFFF);

    // Körper
    pGegnerGrafix[GegnerArt]->RenderSprite(-TileEngine.XOffset + xPos,
                                           -TileEngine.YOffset + yPos + yoff, AnimPhase,
                                           0xFFFFFFFF);

    if (AlreadyDrawn == false) {
        DirectGraphics.SetAdditiveMode();
        if (AnimPhase < 5)
            Projectiles.LavaFlare.RenderSprite(-TileEngine.XOffset + xPos - 70.0f + static_cast<float>(a * 4),
                                               -TileEngine.YOffset + yPos - 10.0f + yoff, 0, 0xBBFFAA66);

        if (AnimPhase > 5)
            Projectiles.LavaFlare.RenderSprite(-TileEngine.XOffset + xPos + 60.0f - static_cast<float>((a - 5) * 4),
                                               -TileEngine.YOffset + yPos - 10.0f + yoff, 0, 0xBBFFAA66);
        DirectGraphics.SetColorKeyMode();
    }

    AlreadyDrawn = true;
}

// --------------------------------------------------------------------------------------
// "Bewegungs KI"
// --------------------------------------------------------------------------------------

void GegnerZitrone::DoKI() {
    blocko = TileEngine.BlockOben(xPos, yPos, xPosOld, yPosOld, GegnerRect[GegnerArt], true);
    blocku = TileEngine.BlockUnten(xPos, yPos, xPosOld, yPosOld, GegnerRect[GegnerArt], true);
    blockl = TileEngine.BlockLinks(xPos, yPos, xPosOld, yPosOld, GegnerRect[GegnerArt], true);
    blockr = TileEngine.BlockRechts(xPos, yPos, xPosOld, yPosOld, GegnerRect[GegnerArt], true);

    CalcKnarreWinkel();

    // Wackeln lassen
    WackelOffset += Timer.sync(0.25f);
    while (WackelOffset > TWO_PI)
        WackelOffset -= TWO_PI;

    // Richtige Animationsphase setzen
    // abhängig von der Position zum Spieler
    //
    float dummy = (pAim->xpos + 35) - (xPos + 57);
    dummy /= 50.0f;

    AnimPhase = 5 - static_cast<int>(dummy);

    AnimPhase = std::clamp(AnimPhase, 0, 10);

    if (blockl & BLOCKWERT_WAND || blockr & BLOCKWERT_WAND)
        xSpeed = 0.0f;

    if (blocko & BLOCKWERT_WAND || blocku & BLOCKWERT_WAND)
        ySpeed = 0.0f;

    switch (Handlung) {
        // In der Luft rumdümpeln
        //
        case GEGNER::STEHEN: {
            // Abstand zu groß?
            // Oder getroffen?
            // Dann gleich hinterherflitzen
            if (PlayerAbstand() > 300)  // ||
                                        //				DamageTaken > 0.0f)
                FollowCount = 0.0f;

            // neuen punkt aussuchen?
            //
            FollowCount -= Timer.sync(1.0f);

            if (FollowCount < 0.0f) {
                FollowCount = 20.0f;

                int dx = random(60) + 30;
                if (random(2) == 0)
                    dx *= -1;

                NewX = pAim->xpos + dx;
                NewY = pAim->ypos - random(80) - 80.0f;

                if (NewX > xPos)
                    xAcc = 5.0f;
                else
                    xAcc = -5.0f;

                if (NewY > yPos)
                    yAcc = 5.0f;
                else
                    yAcc = -5.0f;

                Handlung = GEGNER::LAUFEN;
            }

            // Schiessen
            ShotDelay -= Timer.sync(1.0f);

            if (ShotDelay < 0.0f) {
                ShotDelay = 5.0f;

                WinkelUebergabe = 360.0f - KnarreWinkel;
                // DKS - Support new trig sin/cos lookup table and use deg/rad versions of sin/cos:
                // Projectiles.PushProjectile(xPos + 53.0f - (float)sin((180 - KnarreWinkel) / 180.0f * PI) * 45.0f,
                //                             yPos + 56.0f + (float)cos((180 - KnarreWinkel) / 180.0f * PI) * 45.0f +
                //                             static_cast<float>(sin(WackelOffset) * 10.0f), FLUGLASER);
                Projectiles.PushProjectile(xPos + 53.0f - sin_deg(180.0f - KnarreWinkel) * 45.0f,
                                           yPos + 56.0f + cos_deg(180.0f - KnarreWinkel) * 45.0f +
                                               static_cast<float>(sin(WackelOffset) * 10.0f),
                                           FLUGLASER);

                SoundManager.PlayWave(100, 128, 24000 + random(500), SOUND::LASERSHOT);
            }
        } break;

        // Zu neuem Punkt hinfliegen
        //
        case GEGNER::LAUFEN: {
            // speed begrenzen
            xSpeed = std::clamp(xSpeed, -20.0f, 20.0f);
            ySpeed = std::clamp(ySpeed, -20.0f, 20.0f);

            // xpunkt erreicht?
            if (xSpeed < 0.0f && xPos < NewX) {
                xAcc = 0.0f;

                xSpeed += Timer.sync(7.0f);
                if (xSpeed > 0.0f)
                    xSpeed = 0.0f;
            }

            if (xSpeed > 0.0f && xPos > NewX) {
                xAcc = 0.0f;

                xSpeed -= Timer.sync(7.0f);
                if (xSpeed < 0.0f)
                    xSpeed = 0.0f;
            }

            // ypunkt erreicht?
            if (ySpeed < 0.0f && yPos < NewY) {
                yAcc = 0.0f;

                ySpeed += Timer.sync(7.0f);
                if (ySpeed > 0.0f)
                    ySpeed = 0.0f;
            }

            if (ySpeed > 0.0f && yPos > NewY) {
                yAcc = 0.0f;

                ySpeed -= Timer.sync(7.0f);
                if (ySpeed < 0.0f)
                    ySpeed = 0.0f;
            }

            // Punkt komplett erreicht?
            if (xSpeed == 0.0f && ySpeed == 0.0f) {
                Handlung = GEGNER::STEHEN;
                FollowCount = 20.0f;
                ShotDelay = 5.0f;
            }
        } break;

        // Drone stürzt ab
        //
        case GEGNER::FALLEN: {
            // An die Wand gekracht ?
            if (blockl & BLOCKWERT_WAND || blockr & BLOCKWERT_WAND || blocko & BLOCKWERT_WAND ||
                blocku & BLOCKWERT_WAND || blocku & BLOCKWERT_PLATTFORM)
                Energy = 0.0f;

            // Grenze der Fallgeschwindigkeit
            if (ySpeed > 35.0f)
                yAcc = 0.0f;

            // Drone rauchen lassen
            SmokeDelay -= Timer.sync(1.0f);
            if (SmokeDelay <= 0.0f) {
                SmokeDelay = 0.1f;
                PartikelSystem.PushPartikel(xPos + 20.0f + static_cast<float>(random(30)),
                                            yPos + 10.0f + static_cast<float>(random(40)), SMOKE);
                PartikelSystem.PushPartikel(xPos + 20.0f + static_cast<float>(random(30)),
                                            yPos + 10.0f + static_cast<float>(random(40)), SMOKE3);
            }
        } break;
    }

    // Drone stürzt ab ?
    //
    if (Energy <= 0.0f && Handlung != GEGNER::FALLEN) {
        Handlung = GEGNER::FALLEN;
        AnimCount = 0.0f;
        Energy = 200.0f;
        ySpeed = 4.0f;
        yAcc = 2.0f;
        xAcc = 0.0f;
        xSpeed = 5.0f;

        if (random(2) == 0)
            xSpeed *= -1;

        SoundManager.PlayWave(100, 128, 8000 + random(4000), SOUND::EXPLOSION4);

        for (int i = 0; i < 8; i++) {
            PartikelSystem.PushPartikel(xPos + static_cast<float>(random(80)),
                                        yPos - 10.0f + static_cast<float>(random(90)), EXPLOSION_MEDIUM2);
            PartikelSystem.PushPartikel(xPos + static_cast<float>(random(80)),
                                        yPos - 10.0f + static_cast<float>(random(90)), SPIDERSPLITTER);
        }
    }
}

// --------------------------------------------------------------------------------------
// Drone explodiert
// --------------------------------------------------------------------------------------

void GegnerZitrone::GegnerExplode() {
    SoundManager.PlayWave(100, 128, 8000 + random(4000), SOUND::EXPLOSION4);

    for (int i = 0; i < 12; i++) {
        PartikelSystem.PushPartikel(xPos + static_cast<float>(random(80)),
                                    yPos - 10.0f + static_cast<float>(random(90)), EXPLOSION_MEDIUM2);
        PartikelSystem.PushPartikel(xPos + static_cast<float>(random(80)),
                                    yPos - 10.0f + static_cast<float>(random(90)), SPIDERSPLITTER);
        PartikelSystem.PushPartikel(xPos + 10.0f + static_cast<float>(random(80)),
                                    yPos - 10.0f + static_cast<float>(random(90)), SCHROTT1);
    }

    for (int i = 0; i < 4; i++)
        PartikelSystem.PushPartikel(xPos - 30.0f + static_cast<float>(random(70)),
                                    yPos - 30.0f + static_cast<float>(random(80)), SPLITTER);

    Player[0].Score += 400;
}
