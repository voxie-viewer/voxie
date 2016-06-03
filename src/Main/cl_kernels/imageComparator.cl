/*
*
* m11 m12 0
* m21 m22 0
* dx  dy  1
*
*/

__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;


typedef struct{
        float scaleX;
        float scaleY;
        float dx;
        float dy;
        float angleX;
        float angleY;
} rectangleData;

typedef struct
{
        float scaleX;
        float scaleY;
        float dx;
        float dy;
        float angleX;
        float angleY;
} ellipseData;

typedef struct
{
        float x;
        float y;
} polygonPoint;


bool pointInRect(__constant rectangleData* rectangle, int size, float x, float y)
{
bool result = false;
for (int i = 0; i < size && !result; i++)
{
    float newX = (rectangle[i].scaleX * x) + (rectangle[i].angleX * y) + rectangle[i].dx;
    float newY = (rectangle[i].scaleY * y) + (rectangle[i].angleY * x) + rectangle[i].dy;

    if (newX >= 0 && newX <= 1 && newY >= 0 && newY <= 1)
    {
        result = true;
    }
}

return result;
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

        temp = rX;
        rX = pX;
        pX  = temp;
    }
    if (qY == pY && qX == pX)
    {
        return 0;
    }
    if ((qY <= pY)||(qY > rY))
    {
        return 1;
    }
    float delta = (pX - qX) * (rY - qY) - (pY - qY) * (rX - qX);
    if (delta > 0.0)
    {
        return -1;
    } else if(delta < 0){
        return 1;
    } else {
        return 0;
    }
}


bool pointInPoly(__constant int* polyOffset, int offsetSize, __constant polygonPoint* polyPoint, float x, float y)
{
for (int i = 0; i < offsetSize; i++)
{
    int start = polyOffset[i];
    int size = polyOffset[i + 1];
    int t = -1;
    for (; start < (size) - 1; start++)
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


bool pointInEllipse(__constant ellipseData* ellipse, int size, float x, float y)
{
bool result = false;
for (int i = 0; i < size && !result; i++)
{
    float newX = ellipse[i].scaleX * x + ellipse[i].angleX * y + ellipse[i].dx;
    float newY = ellipse[i].scaleY * y + ellipse[i].angleY * x + ellipse[i].dy;

    float temp = (newX * newX) + (newY * newY);
    float radius = sqrt(temp);

    if (radius <= 1.0)
    {
        result = true;
    }
}
return result;
}

__kernel void imageComparator(__global float* source, __global float* filtered, __constant rectangleData* rectangle, int rectSize,
                                 __constant ellipseData* ellipse, int ellipseSize, __constant int* polyOffset, int offsetSize, __constant polygonPoint* polyPoint,
                                float4 area)
{
int x = get_global_id(0);
int y = get_global_id(1);
int imgW = get_global_size(0);

float relX = area.z;
float relY = area.w;

float planeX = (x * relX) + area.x;
float planeY = (y * relY) + area.y;

if (!pointInRect(rectangle, rectSize, planeX, planeY) && !pointInPoly(polyOffset, offsetSize, polyPoint, planeX, planeY) && !pointInEllipse(ellipse, ellipseSize, planeX, planeY))
{
    //write_imagef(filtered, (int2){x, y}, read_imagef(source, sampler, (int2){x, y}));
    filtered[y*imgW + x] = source[y*imgW + x];
}

}


// Local Variables:
// mode: c
// tab-width: 4
// c-basic-offset: 4
// End:
