//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#include <cmath>
#include "os.h"
#include "Podium.h"
#include "Vector.h"

TPodium::TPodium()
{
    delta = twopi / 50;
    xrad = 1.6f;
    yrad = 1.6f;
    trad = 1.6f;
    y = -1;
    thickness = 1;
}

void TPodium::Draw() const
{
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);

    // Draw the top with 30% transparency to allow the reflection to show through.
    glColor4f(1.0F, 1.0F, 1.0F, 0.7F);
    DrawTop();

    // Draw the sides.
    glEnable(GL_LIGHTING);
    DrawSide();
    glDisable(GL_LIGHTING);

    // Draw the outline of the top.
    glColor4f(1.0F, 1.0F, 1.0F, 1.0F);
    DrawTop(true);

    // Draw the bottom and its outline.
    DrawBottom();
    DrawBottom(true);

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

void TPodium::DrawSide() const
{
    glBegin(GL_QUAD_STRIP);
    glTexCoord2f(0, 0);
    for (float theta = twopi + delta / 2; theta >= 0; theta -= delta) {
        glNormal3f(cosf(theta), 0, sinf(theta));
        glTexCoord2f(theta, 0);
        glVertex3f(xrad * cosf(theta), y, yrad * sinf(theta));
        glTexCoord2f(theta, thickness);
        glVertex3f(xrad * cosf(theta), y - thickness, yrad * sinf(theta));
    }
    glEnd();
}

void TPodium::drawCircle(float y, bool outline, bool clockwise) const
{
    if (outline) {
        glLineWidth(2);
        glBegin(GL_LINE_STRIP);
        drawCircle(y, clockwise);
        glEnd();
    } else {
        glBegin(GL_TRIANGLE_FAN);
        glTexCoord2f(0, 0);
        glVertex3f(0, y, 0);
        drawCircle(y, clockwise);
        glEnd();
    }
}

void TPodium::drawCircle(float y, bool clockwise) const
{
    if (clockwise) {
        for (float theta = 0; theta < twopi + delta / 2; theta += delta) {
            glTexCoord2f(trad * cosf(theta), trad * sinf(theta));
            glVertex3f(xrad * cosf(theta), y, yrad * sinf(theta));
        }
    } else {
        for (float theta = twopi + delta / 2; theta >= 0; theta -= delta) {
            glTexCoord2f(trad * cosf(theta), trad * sinf(theta));
            glVertex3f(xrad * cosf(theta), y, yrad * sinf(theta));
        }
    }
}
