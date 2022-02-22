/*
 * Copyright (c) 2014-2022 The Voxie Authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
*
* m11 m12 0
* m21 m22 0
* dx  dy  1
*
*/

//const sampler_t sampler = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT | CLK_FILTER_NEAREST;

struct rectangleData
{
        double scaleX = 1;
        double scaleY = 1;
        double dx = 0;
        double dy = 0;
        double angleX = 0;
        double angleY = 0;
};

struct ellipseData
{
        double scaleX = 1;
        double scaleY = 1;
        double dx = 0;
        double dy = 0;
        double angleX = 0;
        double angleY = 0;
};

struct polygonPoint
{
        double x;
        double y;
};

bool poinInRect(rectangleData* rectangle, int size, float x, float y)
{
for (int i = 0; i < size, i++)
{
    float newX = rectangle[i].scaleX * x + y * rectangle[i].angleX * y + dx;
    float newY = rectangle[i].scaleY * y + rectangle[i].angleY * x + dy;

if (newX >= 0 && newX <= 1 && newY >= 0 && newY <= 1)
{
    return true;
}
}
return false;
}

int kreuzProdTest(float qX, float qY, float pX, float pY, float rX, float rY)
{
    int result = 0;
    if ((qY == pY) &&(qY == rY) && (pY == rY))
    {
        if(((pX <= qX) && (qX <= rX))||((rX <= qX) && (qX <= pX)))
        {
            return 0;
        } else {
            return 1;
        }
    }
    if ( pY > rY)
    {
        float temp = rY;
        rY = pY;
        pY = temp;
    }
    if (qY == pY && qX == pX)
    {
        return 0;
    }
    if ((qY <= pY)||(qY > rY))
    {
        return 1;
    }
    float delta = (pX - qX) * (rY * qY) - (pY - qY) * (rX - qX);
    if (delta > 0.0)
    {
        return -1;
    } else if(delta < 0){
        return 1;
    } else {
        return 0;
    }
}


boolean pointInPoly(__global int* polyOffset, int offsetSize, __global polygonPoint* polyPoint, float x, float y)
{
for (int i = 0; i < offsetSize - 1; i++)
{
    int start = polyOffset[i];
    int size = polyOffset[i + 1];
    int t = -1;
    for (; start < size - 1; start++)
    {
        t = t * kreuzProdTest(x, y, polyPoint[start].x, polyPoint[start].y, polyPoint[start + 1].x, polyPoint[start + 1].y);
    }
    if (1 == t || 0 == t)
    {
        return true;
    }
}

return false;

}


bool pointInEllipse(ellipseData* ellipse, int size, float x, float y)
{

for (int i = 0; i < size, i++)
{
    float newX = ellipse[i].scaleX * x + y * ellipse[i].angleX * y + dx;
    float newY = ellipse[i].scaleY * y + ellipse[i].angleY * x + dy;

if (newX >= 0 && newX <= 1 && newY >= 0 && newY <= 1)
{
    return true;
}
}
return false;
}



__kernel void imageComparator(__read_only image2d_t source, __read_only image2d_t filtered, __global rectangleData* rectangle,
                                int rectSize, __global ellipseData* ellipse, int ellipseSize, __global int* polyOffset, int offsetSize, __global polygonPoint* polyPoint,
                              float startX, float startY, float width, float height, int imageWidth, int imageHeight, sampler_t sampler)
{

int x = get_global_id(0);
int y = get_global_id(1);

float relX =  width / imageWidth;
float relY = height / imageHeight;
float X = (x * relX) + startX;
float Y = (y * relY) + startY;

if (pointInRect(rectangle, rectSize, X, Y) || pointInPoly(polyOffset, offsetSize, polyPoint, X, Y) || pointInEllipse(ellipse, ellipseSize, X, Y))
{
    write_imagef(source, (int2){x,y}, read_imagef(filtered, sampler, (int2){x, y}));
}
}


// Local Variables:
// mode: c
// tab-width: 4
// c-basic-offset: 4
// End:
