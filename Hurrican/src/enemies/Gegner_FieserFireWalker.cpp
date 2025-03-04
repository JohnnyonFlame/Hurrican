// --------------------------------------------------------------------------------------
// Der Fiese Walker mit dem Flammenwerfer
// --------------------------------------------------------------------------------------

#include "Gegner_FieserFireWalker.hpp"
#include "stdafx.hpp"

// --------------------------------------------------------------------------------------
// Konstruktor
// --------------------------------------------------------------------------------------

GegnerFieserFireWalker::GegnerFieserFireWalker(int Wert1, int Wert2, bool Light) {
    Handlung = GEGNER::LAUFEN;
    Energy = 120;
    Value1 = Wert1;
    Value2 = Wert2;
    ChangeLight = Light;
    Destroyable = true;
    AnimSpeed = 0.5f;
    AnimEnde = 14;
    xSpeed = 5.0f;
    ShotDelay = 30.0f;
    ShotDelay2 = 0.0f;
    OwnDraw = true;
}

// --------------------------------------------------------------------------------------
// Eigene Draw Funktion
// --------------------------------------------------------------------------------------

void GegnerFieserFireWalker::DoDraw() {
    // Gegner rendern
    int Wert = 255 - static_cast<int>(DamageTaken);
    bool mirror = (BlickRichtung != DirectionEnum::LINKS);

    D3DCOLOR Color = D3DCOLOR_RGBA(255, Wert, Wert, 255);
    pGegnerGrafix[GegnerArt]->RenderSprite(xPos - TileEngine.XOffset,
                                           yPos - TileEngine.YOffset, AnimPhase, Color, mirror);

    if (AlreadyDrawn)
        return;

    if (Handlung == GEGNER::STEHEN) {
        // Leuchten beim Schiessen rendern
        //
        DirectGraphics.SetAdditiveMode();
        Projectiles.LavaFlare.RenderSprite(xPos - TileEngine.XOffset - 30.0f +
                                               static_cast<float>(6 + Direction::asInt(BlickRichtung) * 36),
                                           yPos - TileEngine.YOffset - 30.0f, 0, 0xFFFF8822);
        DirectGraphics.SetColorKeyMode();
    }

    AlreadyDrawn = true;
}

// --------------------------------------------------------------------------------------
// "Bewegungs KI"
// --------------------------------------------------------------------------------------

void GegnerFieserFireWalker::DoKI() {
    SimpleAnimation();

    // Je nach Handlung anders verhalten
    //
    switch (Handlung) {
        // rumhopsen
        //
        case GEGNER::LAUFEN: {
            if (!(blocku & BLOCKWERT_WAND) && !(blocku & BLOCKWERT_PLATTFORM)) {
                Handlung = GEGNER::FALLEN;
                ySpeed = 2.0f;
                yAcc = 3.0f;
            }

            // nahe genug zum schiessen?
            //
            if (ShotDelay <= 0.0f && AnimPhase == 3 && PlayerAbstand() < 400 && PlayerAbstandVert() < 150 &&
                ((BlickRichtung == DirectionEnum::RECHTS &&
                  xPos + GegnerRect[GegnerArt].right < pAim->xpos + pAim->CollideRect.left) ||
                 (BlickRichtung == DirectionEnum::LINKS &&
                  xPos + GegnerRect[GegnerArt].left > pAim->xpos + pAim->CollideRect.right))) {
                ShotDelay = 40.0f;
                ShotDelay2 = 0.0f;
                Handlung = GEGNER::STEHEN;
                xSpeed = 0.0f;
                AnimEnde = 0;

                SoundManager.PlayWave(100, 128, 11025, SOUND::FEUERFALLE);
            }

            if (ShotDelay > 0.0f)
                ShotDelay -= Timer.sync(1.0f);

            if (TurnonShot())
                ShotDelay = -1.0f;

        } break;

        // Flammenwerfer schiessen
        //
        case GEGNER::STEHEN: {
            if (ShotDelay2 > 0.0f)
                ShotDelay2 -= Timer.sync(1.0f);

            if (ShotDelay2 <= 0.0f) {
                ShotDelay2 = 0.4f;
                ShotDelay -= 1.0f;

                Projectiles.PushProjectile(xPos + 5.0f + static_cast<float>(Direction::asInt(BlickRichtung) * 38),
                                           yPos - 7.0f, WALKERFIRE, pAim);
            }

            // Spieler nicht mehr vor dem Walker? Dann auch nicht mehr schiessen
            if ((BlickRichtung == DirectionEnum::RECHTS && xPos + 60 >= pAim->xpos) ||
                (BlickRichtung == DirectionEnum::LINKS && xPos <= pAim->xpos + 80))
                ShotDelay = -1.0f;

            if (ShotDelay < 0.0f) {
                ShotDelay = 20.0f;
                Handlung = GEGNER::LAUFEN;
                AnimEnde = 14;
                xSpeed = 5.0f * Direction::asInt(BlickRichtung);
            }

        } break;

        case GEGNER::FALLEN: {
            if (ySpeed > 35.0f) {
                ySpeed = 35.0f;
                yAcc = 0.0f;
            }

            if (blocku & BLOCKWERT_WAND || blocku & BLOCKWERT_PLATTFORM) {
                ySpeed = 0.0f;
                yAcc = 0.0f;
                Handlung = GEGNER::LAUFEN;
            }

        } break;
    }

    TurnonWall();

    // Spieler berührt den Gegner?
    //
    TestDamagePlayers(Timer.sync(4.0f));
}

// --------------------------------------------------------------------------------------
// FieserFireWalker explodiert
// --------------------------------------------------------------------------------------

void GegnerFieserFireWalker::GegnerExplode() {
    PartikelSystem.PushPartikel(xPos - 30.0f, yPos - 30.0f, EXPLOSION_BIG);

    for (int i = 0; i < 8; i++)
        PartikelSystem.PushPartikel(xPos - 30.0f + static_cast<float>(random(60)),
                                    yPos - 30.0f + static_cast<float>(random(60)), EXPLOSION_MEDIUM2);
    for (int i = 0; i < 12; i++)
        PartikelSystem.PushPartikel(xPos + static_cast<float>(random(50)),
                                    yPos + static_cast<float>(random(50)), SPIDERSPLITTER);

    SoundManager.PlayWave(100, 128, 8000 + random(4000), SOUND::EXPLOSION4);  // Sound ausgeben

    Player[0].Score += 80;
}
