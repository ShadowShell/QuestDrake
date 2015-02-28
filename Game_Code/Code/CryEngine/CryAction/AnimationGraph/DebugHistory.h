// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#ifndef __DebugHistory_H__
#define __DebugHistory_H__

#pragma once

#include "IDebugHistory.h"

//--------------------------------------------------------------------------------

extern void Draw2DLine(float x1, float y1, float x2, float y2, ColorF color, float fThickness = 1.f);

//--------------------------------------------------------------------------------

class CDebugHistory : public IDebugHistory
{
public:

	CDebugHistory(const char* name, int size);
	~CDebugHistory();
	
	VIRTUAL void SetName(const char* newName);

	VIRTUAL void SetVisibility(bool show);

	VIRTUAL void SetupLayoutAbs(float leftx, float topy, float width, float height, float margin);
	VIRTUAL void SetupLayoutRel(float leftx, float topy, float width, float height, float margin);
	VIRTUAL void SetupScopeExtent(float outermin, float outermax, float innermin, float innermax);
	VIRTUAL void SetupScopeExtent(float outermin, float outermax);
	VIRTUAL void SetupColors(ColorF curvenormal, ColorF curveclamped, ColorF box, ColorF gridline, ColorF gridnumber, ColorF name);
	VIRTUAL void SetGridlineCount(int nGridlinesX, int nGridlinesY);

	VIRTUAL void AddValue(float value);
	VIRTUAL void ClearHistory();

  void GetMemoryStatistics(ICrySizer * s) { s->Add(*this); }
	void GetMemoryUsage(ICrySizer *pSizer) const { /*nothing*/ }
	void Render();
	bool IsVisible() const { return m_show; }

	void SetDefaultValue( float x ) { m_hasDefaultValue = true; m_defaultValue = x; }

private:

	void UpdateExtent();
	void UpdateGridLines();

	const char* m_szName;

	bool m_show;

	Vec2 m_layoutTopLeft;
	Vec2 m_layoutExtent;
	float m_layoutMargin;

	float m_scopeOuterMax;
	float m_scopeOuterMin;
	float m_scopeInnerMax;
	float m_scopeInnerMin;
	float m_scopeCurMax;
	float m_scopeCurMin;

	ColorF m_colorCurveNormal;
	ColorF m_colorCurveClamped;
	ColorF m_colorBox;
	ColorF m_colorGridLine;
	ColorF m_colorGridNumber;
	ColorF m_colorName;

	int m_wantedGridLineCountX;
	int m_wantedGridLineCountY;
	static const uint8 GRIDLINE_MAXCOUNT = 10;
	int m_gridLineCount;
	float m_gridLine[GRIDLINE_MAXCOUNT];

	float* m_pValues;
	int m_maxCount;
	int m_head;
	int m_count;

	int m_scopeRefreshDelay;
	int m_gridRefreshDelay;

	bool m_hasDefaultValue;
	float m_defaultValue;
	bool m_gotValue;
};

//--------------------------------------------------------------------------------

class CDebugHistoryManager : public IDebugHistoryManager
{

	typedef string MapKey;
	typedef CDebugHistory* MapValue;
	typedef std::map<MapKey, MapValue> Map;
	typedef std::pair<MapKey, MapValue> MapEntry;
	typedef Map::iterator MapIterator;

public:

	CDebugHistoryManager()
	{
		Clear();

		if (!m_allhistory)
			m_allhistory = new std::set<CDebugHistoryManager*>();
		m_allhistory->insert(this);
	}

	~CDebugHistoryManager()
	{
		Clear();

		m_allhistory->erase(this);
	}

	VIRTUAL IDebugHistory* CreateHistory(const char* id, const char* name=0);
	VIRTUAL void RemoveHistory(const char* id);

	VIRTUAL IDebugHistory* GetHistory(const char* id);
	VIRTUAL void Clear()
	{
		MapIterator it = m_histories.begin();
		while (it != m_histories.end())
		{
			CDebugHistory* history = (*it).second;
			delete history;

			++it;
		}

		m_histories.clear();
	}
  virtual void Release();
  VIRTUAL void GetMemoryUsage(ICrySizer * s) const;

	static void RenderAll();
	static void SetupRenderer();

	void LayoutHelper(const char* id, const char* name, bool visible, float minout, float maxout, float minin, float maxin, float x, float y, float w=1.0f, float h=1.0f);

private:
	void Render(bool bSetupRenderer=false);

	Map m_histories;
	static std::set<CDebugHistoryManager*> * m_allhistory;
};

//--------------------------------------------------------------------------------

#endif // __DebugHistory_H__
