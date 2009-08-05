//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#define _USE_MATH_DEFINES
#include "App.h"
#include <math.h>
#include <stdio.h>
#include "Alien.h"
#include "Frame.h"
#include "os.h"

// The alien's coordinates are specified in [-300,+300] for convenience.
const float Extent = 300;

// The ellipsoids for the eyes are elongated along the y-axis.
const vec2 TEllipsoid::radii(10, 20);

// Define the points along the surface of revolution.
const float data[8][2] =
{
    {0,   240},  // 0
    {100, 240},  // 1
    {70,  140},  // 2
    {5,   140},  // 3
    {5,   90},   // 4
    {100, 40},   // 5
    {125, -130}, // 6
    {0,   -110}, // 7
};

#define BLANK {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}

// x, y, jointWidth, radius, length0, length1, angle0, angle1, transform0, transform1
TJoint joints[TAlien::NumJoints] =
{
    {37.5, 227.5, .5, 4, 60,  40,  40,  -20, BLANK, BLANK}, // right antenna
    {-37.5,227.5, .5, 4, 60,  40,  135,  20, BLANK, BLANK}, // left antenna
    {60,   30,    .5, 6, 100, 50,  45,  -20, BLANK, BLANK}, // right arm
    {-60,  30,    .5, 6, 100, 50,  135,  20, BLANK, BLANK}, // left arm
    {50,  -100,   .5, 10, 50, 150, -80, -10, BLANK, BLANK}, // right leg
    {-50, -100,   .5, 10, 50, 150, -100, 10, BLANK, BLANK}, // left leg
};

TAlien::TAlien(unsigned glid, Id id) : TModel(glid, id)
{
    for (int i = 0; i < 5; ++i) {
        animation.push_back(TAlienMotion(1.5));
        animation.back().add(RightArmBase, -5);
        animation.back().add(RightArmExt, -25);

        animation.push_back(TAlienMotion(1.5));
        animation.back().add(RightArmBase, 5);
        animation.back().add(RightArmExt, 25);
    }

    animation.push_back(TAlienMotion(20));
    animation.back().add(RightArmBase, -5);
    animation.back().add(LeftArmBase, 5);
    animation.back().add(LeftArmExt, -10);
    animation.back().add(RightArmExt, 5);
    
    animation.push_back(TAlienMotion(20));
    animation.back().add(LeftAntennaExt, -5);
    animation.back().add(RightAntennaBase, 5);
    animation.back().add(LeftLegBase, -5);
    animation.back().add(LeftLegExt, 5);

    for (int i = 0; i < 5; ++i) {
        animation.push_back(TAlienMotion(1.5));
        animation.back().add(RightArmBase, -5);
        animation.back().add(RightArmExt, -25);

        animation.push_back(TAlienMotion(1.5));
        animation.back().add(RightArmBase, 5);
        animation.back().add(RightArmExt, 25);
    }

    animation.push_back(TAlienMotion(20));
    animation.back().add(LeftAntennaExt, 5);
    animation.back().add(RightAntennaBase, -5);
    animation.back().add(LeftLegBase, 5);
    animation.back().add(LeftLegExt, -5);

    animation.push_back(TAlienMotion(20));
    animation.back().add(RightArmBase, 5);
    animation.back().add(LeftArmBase, -5);
    animation.back().add(LeftArmExt, 10);
    animation.back().add(RightArmExt, -5);

    memset(surface, 0, sizeof(surface));
}

TAlien::~TAlien()
{
    for (int i = 0; i < NumSurfaces; ++i)
        delete surface[i];
}

void TAlien::Load(int tessFactor)
{
    if (tessFactor != -1)
        slices = tessFactor;

    program = wxGetApp().GetCurrentProgram().GetHandle();

    if (!dirty) {
        wxGetApp().UpdateVerts(numVertices);
        return;
    }

    motion = animation.begin();
    countdown = motion->seconds;

    if (dirty) {
        // Obtain the location of the per-vertex blending weight.
        weightLocation = glGetAttribLocation(program, "weight");
        if (weightLocation == -1)
            wxGetApp().Errorf("GetAttribLocation failed for 'weight'.\n");

        // Set the default weight to 1.
        glVertexAttrib1f(weightLocation, 1.0f);

        // Delete old surfaces (needed after refreshing)
        for (int i = 0; i < TAlien::NumSurfaces; ++i)
            delete surface[i];

        // Create the surfaces.
        for (int i = 0; i < TAlien::NumJoints; ++i)
            surface[i] = new TAppendage(joints[i], weightLocation);

        surface[TAlien::NumJoints + 0] = new TRevolveBezier(0, 1, 2, 3);  // head
        surface[TAlien::NumJoints + 1] = new TRevolveLine(3, 4);          // neck
        surface[TAlien::NumJoints + 2] = new TRevolveBezier(4, 5, 6, 7);  // body
        surface[TAlien::NumJoints + 3] = new TEllipsoid(25, 190, 50);     // right eye
        surface[TAlien::NumJoints + 4] = new TEllipsoid(-25, 190, 50);    // left eye

        // Create the uniforms.
        index0 = TUniform("index0", "int", program);
        index1 = TUniform("index1", "int", program);

        char name[] = "transforms[xx]";
        for (int i = 0; i < NumAngles + 1; ++i) {
            sprintf(name, "transforms[%d]", i);
            transforms[i] = TUniform(name, "mat4", program);
        }

        // Set up some default values for the uniforms and send them to the card.
        index0.Set(0);
        index0.Load();

        index1.Set(0);
        index1.Load();

        // Tweak the starting positions to make it interesting.
        Flex(3, 60);
        Flex(7, 60);
        Flex(11, 10);

        // Generate the transformation matrices for each joint.
        Update();
        dirty = false;
    }

    glNewList(glid, GL_COMPILE);
    numVertices = draw();
    glEndList();
    dirty = false;
    wxGetApp().UpdateVerts(numVertices);
}

void TAlien::Draw() const
{
    for (int i = 0; i < NumAngles + 1; ++i)
        transforms[i].Load();

    glCallList(glid);
}

int TAlien::draw() const
{
    int verts = 0;

    // Draw each appendage using the appropriate transformation matrices.
    for (int i = 0; i < 6; ++i) {
        index0.Set(i * 2 + 1);
        index0.Load();

        index1.Set(i * 2 + 2);
        index1.Load();

        verts += surface[i]->Draw(slices);
    }

    // Turn skinning off.
    glVertexAttrib1f(weightLocation, 1.0f);
    index0.Set(0);
    index0.Load();

    // Draw the head, neck, and body.
    verts += surface[6]->Draw(slices);
    verts += surface[7]->Draw(slices);
    verts += surface[8]->Draw(slices);
    
    // Turn off texturing, then draw the eyes.
    glDisable(GL_TEXTURE_2D);
    static float white[] = {1, 1, 1};
    glUniform3fv(glGetUniformLocation(wxGetApp().GetCurrentProgram().GetHandle(), "SkinColor"), 1, white);
    for (int i = NumSurfaces - 2; i < NumSurfaces; ++i)
        verts += surface[i]->Draw(slices);

    return verts;
}

TJoint& TAlien::GetJoint(int index) const
{
    return ((TAppendage*) surface[index])->joint;
}

TRevolveBezier::TRevolveBezier(int a, int b, int c, int d)
{
    p0.x = data[a][0]; p0.y = data[a][1];
    p1.x = data[b][0]; p1.y = data[b][1];
    p2.x = data[c][0]; p2.y = data[c][1];
    p3.x = data[d][0]; p3.y = data[d][1];
}

TRevolveLine::TRevolveLine(int a, int b)
{
    p0 = vec2(data[a][0], data[a][1]);
    p1 = vec2(data[b][0], data[b][1]);
}

void TRevolveBezier::Eval(vec2& domain, vec3& range)
{
    float u = 1 - domain.u;
    float v = domain.v * twopi;

    // Tweak the texture coordinates.
    domain.u /= 6;

    // Use cubic Bernstein polynomials for the Bezier basis functions.
    float b0 = (1 - u) * (1 - u) * (1 - u);
    float b1 = 3 * u * (1 - u) * (1 - u);
    float b2 = 3 * u * u * (1 - u);
    float b3 = u * u * u;
    vec2 p = p0 * b0 + p1 * b1 + p2 * b2 + p3 * b3;

    range.x = p.x * cosf(v);
    range.z = p.x * sinf(v);
    range.y = p.y;
    range /= Extent;
}

void TRevolveLine::Eval(vec2& domain, vec3& range)
{
    float u = 1 - domain.u;
    float v = domain.v * twopi;

    float radius = p0.x + u * (p1.x - p0.x);
    range.x = radius * cosf(v);
    range.z = radius * sinf(v);
    range.y = p0.y + u * (p1.y - p0.y);
    range /= Extent;
}

void TEllipsoid::Eval(vec2& domain, vec3& range)
{
    float u = fabsf(domain.u * pi);
    float v = fabsf(domain.v * twopi);

    range.x = center.x + radii.x * cosf(v) * sinf(u);
    range.y = center.y + radii.y * sinf(v) * sinf(u);
    range.z = center.z + radii.x * cosf(u);
    range /= Extent;
}

void TAppendage::Eval(vec2& domain, vec3& range)
{
    float v = fabsf((1 - domain.v) * twopi);

    if (domain.u < 0.5) {
        float u = fabsf(domain.u * 2);
        range.x = u * joint.length0;
        range.y = joint.radius * cosf(v);
        range.z = joint.radius * sinf(v);
    } else {
        float u = fabsf((domain.u - 0.5F) * 2);
        range.x = joint.length0 + u * joint.length1;
        range.y = joint.radius * (1 - u) * cosf(v);
        range.z = joint.radius * (1 - u) * sinf(v);
    }
    range /= Extent;
}

//
// Given a parametric value in [0,1], returns a blending weight in [0,1].
//
float TAppendage::CustomAttributeValue(const vec2& domain)
{
    float u = domain.u;

    if (u < 0.5f - joint.width / 2)
        return 1;
    else if (u > 0.5f + joint.width / 2)
        return 0;

    u -= 0.5f - joint.width / 2;
    u /= joint.width;
    u = 1 - u;

    return u;
}

void TAlien::Flex(int index, float delta) const
{
    SetAngle(index, GetAngle(index) + delta);
}

float& TAlien::GetAngle(int index) const
{
    return (index % 2) ? GetJoint(index/2).angle1 : GetJoint(index/2).angle0;
}

void TAlien::SetAngle(int index, float theta) const
{
    if (index < 0 || index >= NumAngles)
        return;

    float& angle = GetAngle(index);
    angle = theta;
}

void TAlien::Reset()
{
    for (int index = 0; index < NumAngles; ++index) {
        float originalAngle = (index % 2) ? GetJoint(index/2).angle1 : GetJoint(index/2).angle0;
        SetAngle(index, originalAngle);
    }
}

mat4& TAlien::GetTransform(int index)
{
    return (index % 2) ? GetJoint(index/2).transform1 : GetJoint(index/2).transform0;
}

// Flip the normals in the center of his eyes to create black pupils.
bool TEllipsoid::Flip(const vec2& domain)
{
    return (domain.u < 0.125f);
}

void TAlien::Update()
{
    if (wxGetApp().Frame()->GetMenuBar()->FindItem(Id::ViewAnimate)->IsChecked())
        animate();

    glPushMatrix();

    // Squish him in Z so he isn't so round and fat.
    glScalef(1, 1, 0.5F);

    // Calculate the modelview-projection matrix.
    mat4 projection;
    mat4 modelview;
    glGetFloatv(GL_PROJECTION_MATRIX, projection.data);
    glGetFloatv(GL_MODELVIEW_MATRIX, modelview.data);
    mat4 mvp = projection * modelview;
    transforms[0].Set(mvp);

    for (int jointIndex = 0; jointIndex < NumJoints; ++jointIndex) {
        TJoint& joint = GetJoint(jointIndex);
        glPushMatrix();
        glTranslatef(joint.x / Extent, joint.y / Extent, 0);
        glRotatef(joint.angle0, 0, 0, 1);
        glGetFloatv(GL_MODELVIEW_MATRIX, joint.transform0.data);
        glTranslatef(joint.length0 / Extent, 0, 0);
        glRotatef(joint.angle1, 0, 0, 1);
        glTranslatef(-joint.length0 / Extent, 0, 0);
        glGetFloatv(GL_MODELVIEW_MATRIX, joint.transform1.data);
        glPopMatrix();

        transforms[1 + jointIndex * 2].Set(projection * joint.transform0);
        transforms[2 + jointIndex * 2].Set(projection * joint.transform1);
    }

    glPopMatrix();
}

void TAlien::animate()
{
    if (animation.empty())
        return;

    const float interval = 100;

    countdown -= interval;

    if (countdown <= 0) {
        if (++motion == animation.end()) {
            motion = animation.begin();
            Reset();
        }
        countdown = 1000 * motion->seconds;
    }

    for (TAlienMovements::const_iterator i = motion->movements.begin(); i != motion->movements.end(); ++i)
        Flex(i->joint, i->dps * interval / 1000.0f);
}
