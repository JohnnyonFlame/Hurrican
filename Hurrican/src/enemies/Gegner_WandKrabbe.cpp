// --------------------------------------------------------------------------------------
// Die Wandkrabbe
//
// krabbelt im Fluglevel an der Wand rum und springt den Hurri an
// --------------------------------------------------------------------------------------

#include "Gegner_WandKrabbe.hpp"
#include "stdafx.hpp"

// --------------------------------------------------------------------------------------
// Konstruktor
// --------------------------------------------------------------------------------------

GegnerWandKrabbe::GegnerWandKrabbe(int Wert1, int Wert2, bool Light) :
    lastPlayerAction(DirectionEnum::LINKS),
    rot(0.0f)
{
    Handlung = GEGNER::LAUFEN;
    Energy = 100;
    ChangeLight = Light;
    Destroyable = true;
    Value1 = Wert1;
    Value2 = Wert2;
    OwnDraw = true;
    AnimEnde = 10;
    AnimStart = 1;
    AnimSpeed = 0.75f;
    ySpeed = 3.0f;

}

// --------------------------------------------------------------------------------------
// Eigene Draw Funktion
// --------------------------------------------------------------------------------------

void GegnerWandKrabbe::DoDraw() {
    bool mirrored = (BlickRichtung != DirectionEnum::RECHTS);

    // gerade im Flug? Dann rotiert rendern
    if (Handlung == GEGNER::DREHEN || Handlung == GEGNER::FALLEN || Handlung == GEGNER::EXPLODIEREN)
        pGegnerGrafix[GegnerArt]->RenderSpriteRotated(xPos - TileEngine.XOffset,
                                                      yPos - TileEngine.YOffset, rot, AnimPhase,
                                                      0xFFFFFFFF);
    // andernfalls normal an der Wand entlang rendern
    else {
        pGegnerGrafix[GegnerArt]->itsRect = pGegnerGrafix[GegnerArt]->itsPreCalcedRects[AnimPhase];
        pGegnerGrafix[GegnerArt]->RenderMirroredSprite(xPos - TileEngine.XOffset,
                                                       yPos - TileEngine.YOffset, 0xFFFFFFFF,
                                                       mirrored, ySpeed < 0.0f);
    }
}

// --------------------------------------------------------------------------------------
// "Bewegungs KI"
// --------------------------------------------------------------------------------------

void GegnerWandKrabbe::DoKI() {
    if (Handlung != GEGNER::DREHEN && Handlung != GEGNER::EXPLODIEREN)
        SimpleAnimation();

    blockl = TileEngine.BlockLinks(xPos, yPos, xPosOld, yPosOld, GegnerRect[GegnerArt], true);
    blockr = TileEngine.BlockRechts(xPos, yPos, xPosOld, yPosOld, GegnerRect[GegnerArt], true);

    if (blockl & BLOCKWERT_WAND)
        BlickRichtung = DirectionEnum::RECHTS;
    else
        BlickRichtung = DirectionEnum::LINKS;

    // an Wänden oben und unten umdrehen
    if (((blocko & BLOCKWERT_WAND || blocko & BLOCKWERT_GEGNERWAND) && ySpeed < 0.0f) ||
        (((blocku & BLOCKWERT_WAND || blocku & BLOCKWERT_GEGNERWAND) && ySpeed > 0.0f) && Handlung == GEGNER::LAUFEN))
        ySpeed *= -1.0f;

    switch (Handlung) {
        // an der Wand entlang laufen
        //
        case GEGNER::LAUFEN: {
            // Gegner in Nähe des Spielers? Dann springen
            //
            if (abs(static_cast<int>(pAim->ypos - yPos)) < 100.0f) {
                float absx = pAim->xpos + 35 - (xPos + 20);
                float absy = pAim->ypos + 40 - (yPos + 20) - 160;

                xSpeed = absx / 10.0f;
                ySpeed = absy / 10.0f;
                yAcc = 3.25f;

                Handlung = GEGNER::DREHEN;
            }
        } break;

        // auf den Spieler zufliegen und sich dabei drehen
        //
        case GEGNER::DREHEN: {
            rot += Timer.sync(static_cast<float>(random(50) + 20));

            clampAngle(rot);

            if (ySpeed > 20.0f)
                ySpeed = 20.0f;

        } break;

        // nach "Abschütteln" runterfallen
        //
        case GEGNER::FALLEN: {
            rot += Timer.sync(static_cast<float>(random(50) + 20));

            clampAngle(rot);

            if (ySpeed > 40.0f)
                ySpeed = 40.0f;

        } break;

        case GEGNER::EXPLODIEREN: {
            rot += Timer.sync(static_cast<float>(random(50) + 20));

            clampAngle(rot);

            if (ySpeed > 40.0f)
                ySpeed = 40.0f;

            // an der Wand abprallen
            //

            if ((blockl & BLOCKWERT_WAND && xSpeed < 0.0f) || (blockr & BLOCKWERT_WAND && xSpeed > 0.0f))
                xSpeed *= -1.0f;

            // rauchen lassen
            //
            AnimCount -= Timer.sync(1.0f);

            while (AnimCount <= 0.0f) {
                AnimCount += 0.2f;

                PartikelSystem.PushPartikel(xPos + 8.0f + static_cast<float>(random(5)),
                                            yPos + 25.0f + static_cast<float>(random(5)), SMOKE3);
                PartikelSystem.PushPartikel(xPos + 8.0f + static_cast<float>(random(5)),
                                            yPos + 28.0f + static_cast<float>(random(5)), FUNKE);
            }

            // bei Aufprall explodieren lassen
            //
            if (blocku & BLOCKWERT_WAND)
                Energy = 0.0f;

        } break;

        case GEGNER::SPECIAL: {
            // Gegner hängt am Spieler
            //
            xPos = pAim->xpos + Value1;
            yPos = pAim->ypos + Value2;

            if (xPos + 15 < pAim->xpos + 35)
                BlickRichtung = DirectionEnum::LINKS;
            else
                BlickRichtung = DirectionEnum::RECHTS;

            // Spieler schüttelt Gegner ab? (schnelles links/rechts drücken)
            // nur, wenn nicht beide gleichzeitig gedrückt werden
            //
            if (!(pAim->Aktion[AKTION_LINKS] == true && pAim->Aktion[AKTION_RECHTS] == true)) {
                if (lastPlayerAction == DirectionEnum::LINKS && pAim->Aktion[AKTION_RECHTS]) {
                    lastPlayerAction = DirectionEnum::RECHTS;
                    Value2 += random(4) + 2;
                }

                if (lastPlayerAction == DirectionEnum::RECHTS && pAim->Aktion[AKTION_LINKS]) {
                    lastPlayerAction = DirectionEnum::LINKS;
                    Value2 += random(4) + 2;
                }

                // Gegner abgeschüttelt?
                //
                if (Value2 > 80) {
                    ySpeed = 5.0f;
                    yAcc = 3.0f;
                    Handlung = GEGNER::FALLEN;
                }
            }

            // Spieler tot? Dann abfallen
            //
            if (pAim->Energy <= 0.0f) {
                ySpeed = 5.0f;
                yAcc = 3.0f;
                Handlung = GEGNER::FALLEN;
            }

        } break;

        default:
            break;
    }  // switch

    // Testen, ob der Spieler die Krabbe berührt hat
    // Wenn ja, dann bleibt sie an ihm hängen
    if (Handlung == GEGNER::DREHEN &&
        SpriteCollision(xPos, yPos, GegnerRect[GegnerArt], pAim->xpos, pAim->ypos, pAim->CollideRect) == true) {
        rot = 0.0f;
        xSpeed = 0.0f;
        ySpeed = 0.0f;
        xAcc = 0.0f;
        yAcc = 0.0f;
        AnimPhase = 0;
        AnimEnde = 0;

        // Spieler Schaden zufügen
        //
        pAim->DamagePlayer(10.0f);

        Handlung = GEGNER::SPECIAL;

        // Position am Spieler merken
        //
        Value1 = static_cast<int>(xPos - pAim->xpos);
        Value2 = static_cast<int>(yPos - pAim->ypos);

        Value1 = std::clamp(Value1, -10, 50);
        Value2 = std::clamp(Value2, -30, 80);
    }

    // Keine Energie mehr, aber noch nicht am "abschmieren"? Dann abschmieren lassen ;)
    //
    if (Handlung != GEGNER::EXPLODIEREN && (Energy <= 0.0f || (blocku & BLOCKWERT_WAND && Handlung != GEGNER::LAUFEN))) {
        Energy = 100.0f;

        AnimEnde = 0;
        AnimPhase = 0;
        AnimCount = 0.1f;

        ySpeed = -15.0f;
        yAcc = 3.0f;

        if (BlickRichtung == DirectionEnum::RECHTS)
            xSpeed = static_cast<float>(random(8) + 4);
        else
            xSpeed = -static_cast<float>(random(8) + 4);

        Handlung = GEGNER::EXPLODIEREN;

        SoundManager.PlayWave(100, 128, 11025, SOUND::EXPLOSION1);
        PartikelSystem.PushPartikel(xPos - 20, yPos, EXPLOSION_MEDIUM2);
    }

    // untem aus dem Screen raus? Dann explodieren
    //
    if (yPos > TileEngine.YOffset + 550.0f)
        Energy = 0.0f;
}

// --------------------------------------------------------------------------------------
// WandKrabbe explodiert
// --------------------------------------------------------------------------------------

void GegnerWandKrabbe::GegnerExplode() {
    SoundManager.PlayWave(100, 128, 11025, SOUND::EXPLOSION1);

    for (int i = 0; i < 6; i++)
        PartikelSystem.PushPartikel(xPos - 20.0f + static_cast<float>(random(10)),
                                    yPos + static_cast<float>(random(30)), EXPLOSION_MEDIUM2);

    for (int i = 0; i < 20; i++) {
        PartikelSystem.PushPartikel(xPos + static_cast<float>(random(30)),
                                    yPos + static_cast<float>(random(64)), FUNKE);
        PartikelSystem.PushPartikel(xPos + static_cast<float>(random(25)),
                                    yPos + static_cast<float>(random(50)), SPIDERSPLITTER);
    }

    Player[0].Score += 100;
}
