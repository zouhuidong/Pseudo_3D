#include "EasyWin32.h"
#include <stdio.h>
#include <math.h>
#include <conio.h>

#define PI 3.1415926

#define SQR(x) ((x)*(x))

struct MAP
{
	int** pMap;
	int w, h;
	int sx, sy;	// 玩家起始位置
}g_map;

int g_nW = 640, g_nH = 480;	// 屏幕大小

int g_nMaxHeight = g_nH;	// 墙体最大高度
int g_nMinHeight = 0;		// 墙体最低高度
int g_nMaxLightness = 200;	// 墙体最大亮度
int g_nMinLightness = 100;	// 墙体最小亮度

double g_dX, g_dY, g_dZ;								// 玩家位置
double g_dViewLength = 15;								// 视野长度
double g_dPlayerAngle = PI / 2;							// 玩家视角角度
double g_dViewAngleRange = PI / 3;						// 视角范围
double g_dHalfViewAngleRange = g_dViewAngleRange / 2;	// 视角范围半宽
double g_dZOffset = 0;									// Z 轴偏移
double g_dMaxZOffset = g_nMaxHeight / 2;				// Z 轴最大偏移

IMAGE g_imgSky;

MAP Init(LPCTSTR pszMapFilePath)
{
	FILE* fp;
	int w, h, x, y;
	int** pMap;
	if (_wfopen_s(&fp, pszMapFilePath, L"r") != 0)
	{
		return {};
	}
	fscanf_s(fp, "%d,%d\n", &w, &h);
	pMap = new int* [w];
	for (int i = 0; i < w; i++)
	{
		pMap[i] = new int[h];
	}
	char* pLine = new char[w + 2];
	for (int i = 0; i < h; i++)
	{
		memset(pLine, 0, w + 2);
		fgets(pLine, w + 2, fp);
		for (int j = 0; j < w; j++)
		{
			pMap[j][i] = (int)pLine[j] - 48;
			if (pMap[j][i] == 2)
			{
				x = j;
				y = i;
			}
		}
	}
	delete[] pLine;

	loadimage(&g_imgSky, L"./sky.jpg");

	return { pMap,w,h,x,y };
}

// 获取距离
bool GetWallDistance(MAP map, double x, double y, double dAngle, double* pDistance)
{
	double dSin = sin(dAngle);
	double dCos = cos(dAngle);
	for (int j = 0;; j++)
	{
		double dX = x + dCos * j;
		double dY = y - dSin * j;
		if (dX < 0 || dX >= g_map.w || dY < 0 || dY >= g_map.h)
		{
			return false;
		}
		else if (g_map.pMap[(int)dX][(int)dY] == 1)
		{
			*pDistance = sqrt(SQR(dX - x) + SQR(dY - y));
			return true;
		}
	}
}

void Render3D(int nImageW, int nImageH)
{
	double dStart = g_dPlayerAngle + g_dHalfViewAngleRange;
	double dEnd = g_dPlayerAngle - g_dHalfViewAngleRange;
	double dAngleIncrease = (dEnd - dStart) / nImageW;
	double dHeightIncrease = (g_nMinHeight - g_nMaxHeight) / g_dViewLength;
	double dLightnessIncrease = (g_nMinLightness - g_nMaxLightness) / g_dViewLength;
	for (int i = 0; i < nImageW; i++)
	{
		double dAngle = dStart + dAngleIncrease * i;
		double dDistance = 0;
		bool isWall = GetWallDistance(g_map, g_dX, g_dY, dAngle, &dDistance);
		if (isWall && dDistance <= g_dViewLength)
		{
			int nHeight = (int)(g_nMaxHeight + dHeightIncrease * dDistance);
			int nLightness = (int)(g_nMaxLightness + dLightnessIncrease * dDistance);
			int nOutY = (int)(
				((((nImageH * g_dZ /* 玩家绘制高度 */) - nHeight / 2) /* 墙体正常绘制位置 */
					+ g_dZOffset / g_dViewLength * dDistance) /* 加上 Z 轴偏移 */
					+ g_dZOffset /* Z 轴偏移后，身高复原 */)
				);
			setlinecolor(RGB(nLightness, nLightness, nLightness));
			line(i, nOutY, i, nOutY + nHeight);
		}
	}
}

// 从原点绘制一条带角度的线段
void DrawAngleLine(double angle, int len)
{
	double dSin = sin(angle);
	double dCos = cos(angle);
	line(0, 0, (int)(len * dCos), (int)(len * dSin));
}

void Render2D()
{
	int nW = 10, nH = 10;
	for (int i = 0; i < g_map.w; i++)
	{
		for (int j = 0; j < g_map.h; j++)
		{
			COLORREF c = WHITE;
			switch (g_map.pMap[i][j])
			{
			case 0:	c = WHITE;	break;
			case 1: c = RED;	break;
			}

			setfillcolor(c);
			solidrectangle(i * nW, j * nH, (i + 1) * nW, (j + 1) * nH);

			if (i == (int)g_dX && j == (int)g_dY)
			{
				setfillcolor(BLUE);
				fillcircle((int)(g_dX * nW), (int)(g_dY * nH), nW / 2);
			}
		}
	}

	setorigin((int)(g_dX * nW), (int)(g_dY * nH));
	setaspectratio(1, -1);

	// 发射扫描线
	setlinecolor(GREEN);
	double dAngleUnit = PI / 64;
	for (double d = g_dHalfViewAngleRange; d >= -g_dHalfViewAngleRange; d -= dAngleUnit)
	{
		double dDistance;
		if (GetWallDistance(g_map, g_dX, g_dY, g_dPlayerAngle + d, &dDistance))
		{
			DrawAngleLine(g_dPlayerAngle + d, (int)(dDistance * nW));
		}
	}

	// 复位
	setlinestyle(PS_SOLID, 1);
	setorigin(0, 0);
	setaspectratio(1, 1);
}

void DrawBackground(int nImageWidth, int nImageHeight)
{
	// 天空
	putimage(
		(int)(-(g_imgSky.getheight() - nImageWidth) / 2 + cos(g_dPlayerAngle) * 100),
		(int)((nImageHeight * g_dZ /* 玩家绘制高度 */) - g_imgSky.getheight() + g_dZOffset * 2),
		&g_imgSky
	);

	// 地面
	setfillcolor(BROWN);
	solidrectangle(0, (int)((nImageHeight * g_dZ /* 玩家绘制高度 */) + g_dZOffset * 2),
		nImageWidth, nImageHeight);
}

void Jump()
{
	static bool isRunning = false;
	double dYOffsetUnit = -20;
	if (!isRunning)
	{
		isRunning = true;
		for (double k = 0.1;; g_dZ += k)
		{
			if (g_dZ >= 0.8)
			{
				k = -0.1;
			}
			else if (g_dZ <= 0.4)
			{
				Sleep(50);
				g_dZOffset -= dYOffsetUnit;
				g_dZ = 0.5;
				break;
			}

			g_dZOffset += k * 10 * dYOffsetUnit;

			Sleep(50);
		}
		isRunning = false;
	}
}

void PlayerControl()
{
	static EasyWin32::MouseDrag drag;

	ExMessage msg;
	while (peekmessage(&msg, EM_MOUSE | EM_KEY))
	{
		switch (EasyWin32::GetExMessageType(msg))
		{
		case EM_MOUSE:
			drag.UpdateMessage(msg);
			if (drag.isRightDrag())
			{
				g_dPlayerAngle -= drag.GetDragX() / 64.0;

				double dZOffset = g_dZOffset - drag.GetDragY() * 3;
				if (abs(dZOffset) < g_dMaxZOffset)
				{
					g_dZOffset = dZOffset;
				}
			}
			break;

		case EM_KEY:
			if (!msg.prevdown)
			{
				bool isMove = true;
				double dForwardAngle = g_dPlayerAngle;
				switch (msg.vkcode)
				{
				case 'W':		break;
				case 'S':		dForwardAngle += PI;			break;
				case 'A':		dForwardAngle += PI / 2;		break;
				case 'D':		dForwardAngle -= PI / 2;		break;
				case VK_SPACE:	std::thread(Jump).detach();		isMove = false;		break;
				case VK_SHIFT:	g_dZ = 0.2;						isMove = false;		break;
				}

				if (isMove)
				{
					double dSin = sin(dForwardAngle);
					double dCos = cos(dForwardAngle);
					double dX = g_dX + dCos;
					double dY = g_dY - dSin;
					if (!(dX < 0 || dX >= g_map.w || dY < 0 || dY >= g_map.h) &&
						g_map.pMap[(int)dX][(int)dY] != 1)
					{
						g_dX = dX;
						g_dY = dY;
					}
				}
			}
			else
			{
				if (msg.message == WM_KEYUP)
				{
					if (msg.vkcode == VK_SHIFT)
					{
						g_dZ = 0.5;
					}
				}
			}
			break;
		}

	}

}

void Tip()
{
	outtextxy(0, 115, L"WSAD SPACE SHIFT");
	outtextxy(0, 135, L"Hold down the right mouse button and drag the camera");
}

int main()
{
	initgraph(g_nW, g_nH);

	g_map = Init(L"map.txt");
	g_dX = g_map.sx;
	g_dY = g_map.sy;
	g_dZ = 0.5;

	while (true)
	{
		BEGIN_TASK();
		cleardevice();
		DrawBackground(g_nW, g_nH);
		Render3D(g_nW, g_nH);
		Render2D();
		Tip();
		PlayerControl();

		if (EasyWin32::isWindowSizeChanged())
		{
			g_nW = getwidth();
			g_nH = getheight();
		}

		END_TASK();
		FLUSH_DRAW();

		Sleep(10);
	}

	closegraph();
	return 0;
}