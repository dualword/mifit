#include "nonguilib.h"
#include "chemlib.h"
#include "maplib.h"
#include "MIDialog.h"

#include "corelib.h"
#include "EMap.h"
#include "id.h"
#include "macafxwin.h"
#include "Application.h"

#include "uitest.h"
#include "MIDialog.h"
#include "MapSettings.h"

struct mmtz_column_;

#ifdef _WIN32
#define _MVS
#define i386
#include <mmtzlib.h>
#undef _MVS
#else
#include <mmtzlib.h>
#endif






void EMap::Save(CArchive& ar, int i) {
  char tmpbuf[1000];
  if (HasPhases()) {
    sprintf(tmpbuf, "MapColumns %s%s %s%s %s%s %s%s %s%s %s%s\n",
            (_fostr.size()?"FO=":""),(_fostr.size()?_fostr.c_str():""),
            (_fcstr.size()?"FC=":""),(_fcstr.size()?_fcstr.c_str():""),
            (_fomstr.size()?"FOM=":""),(_fomstr.size()?_fomstr.c_str():""),
            (_phistr.size()?"PHI=":""),(_phistr.size()?_phistr.c_str():""),
            (_sigfstr.size()?"SIGF=":""),(_sigfstr.size()?_sigfstr.c_str():""),
            (_freeRstr.size()?"FREE=":""),(_freeRstr.size()?_freeRstr.c_str():""));
    ar.Write(tmpbuf,strlen(tmpbuf));
    sprintf(tmpbuf, "LoadMapPhase %d %s\n",i, pathName.c_str());
    ar.Write(tmpbuf,strlen(tmpbuf));

    sprintf(tmpbuf, "unitcell %f %f %f %f %f %f\n", mapheader->a, mapheader->b, mapheader->c, mapheader->alpha, mapheader->beta, mapheader->gamma);
    ar.Write(tmpbuf, strlen(tmpbuf));
    sprintf(tmpbuf, "spacegroupno %d\n", mapheader->spgpno);
    ar.Write(tmpbuf, strlen(tmpbuf));
    /*    sprintf(tmpbuf,"crystal %s\n", mapheader->crystal_name.c_str());
        ar.Write(tmpbuf, strlen(tmpbuf));
        sprintf(tmpbuf,"ctitle %s\n", mapheader->title.c_str());
        ar.Write(tmpbuf, strlen(tmpbuf));
     */
    sprintf(tmpbuf, "coefficients %s\n", StringForMapType(mapheader->maptype));
    ar.Write(tmpbuf, strlen(tmpbuf));
    sprintf(tmpbuf, "fftnx %d\nfftny %d\nfftnz %d\nresmin %0.2f\nresmax %0.2f\n", mapheader->nx,
      mapheader->ny, mapheader->nz, mapheader->resmin, mapheader->resmax);
    ar.Write(tmpbuf, strlen(tmpbuf));
    strcpy(tmpbuf, "fftapply\n");
    ar.Write(tmpbuf, strlen(tmpbuf));
  } else {
    sprintf(tmpbuf, "LoadMap %d %s\n", i, pathName.c_str());
    ar.Write(tmpbuf, strlen(tmpbuf));
  }
  sprintf(tmpbuf, "maptocont %d\n", i);
  ar.Write(tmpbuf, strlen(tmpbuf));
  sprintf(tmpbuf, "maplinewidth %f\n", settings->maplinewidth);
  ar.Write(tmpbuf, strlen(tmpbuf));
  sprintf(tmpbuf, "contourlevels %d\n",
    (settings->MapLevelOn[0] ? 1 : 0) +
    2*(settings->MapLevelOn[1] ? 1 : 0) +
    4*(settings->MapLevelOn[2] ? 1 : 0) +
    8*(settings->MapLevelOn[3] ? 1 : 0) +
    16*(settings->MapLevelOn[4] ? 1 : 0)   );
  ar.Write(tmpbuf, strlen(tmpbuf));
  sprintf(tmpbuf, "contourleveldefault %f %f %f %f %f\n",
    settings->MapLevel[0], settings->MapLevel[1], settings->MapLevel[2], settings->MapLevel[3], settings->MapLevel[4]);
  ar.Write(tmpbuf, strlen(tmpbuf));
  sprintf(tmpbuf, "contourradius %f\n", settings->Radius);
  ar.Write(tmpbuf, strlen(tmpbuf));
  for (int j = 0; j < 5; j++) {
    sprintf(tmpbuf, "color %d\n", settings->MapLevelColor[j]);
    ar.Write(tmpbuf, strlen(tmpbuf));
    sprintf(tmpbuf, "contourcolor %d\n", j+1);
    ar.Write(tmpbuf, strlen(tmpbuf));
  }
  sprintf(tmpbuf, "contourmap %d\n\n", i);
  ar.Write(tmpbuf, strlen(tmpbuf));
}

bool EMap::ContourLevels() {
  MIData data;
  MIContourDialog dlg(0,MapID().c_str());
  MapSettingsToData(*settings, data, mapmin, mapmax);
  if (dlg.GetResults(data)) {
    float mapmin, mapmax;
    DataToMapSettings(data,*settings, mapmin, mapmax);
    SetMapLinewidth(settings->maplinewidth);
    //settings->save(); // No longer relevant.  only the preferences are saved
    mapContourLevelsChanged(this);
    return true;
  }
  return false;
}

long EMap::LoadMap(const char* pathname, int type) {
  if (HasPhases()) {
    if (MIMessageBox("This map already has a reflection list\nDo you want to overwrite it?", "Query", MIDIALOG_YES_NO) == MI_NO) {
      return 0;
    }
  }
  return EMapBase::LoadMap(pathname, type);
}

long EMap::LoadCIFMap(const char* pathname, int datablock) {
  if (refls.size() != 0) {
    if (MIMessageBox("This map already has a reflection list\nDo you want to overwrite it?", "Query", MIDIALOG_YES_NO) == MI_NO) {
      return 0;
    }
  }
  return EMapBase::LoadCIFMap(pathname, datablock);
}

bool EMap::PromptForCrystal() {
  MIData data;
  MISelectCrystalDialog dlg(0, "Select Crystal");
  data["info"].str=CMapHeaderBase().Label();
  dlg.GetResults(data);
  CMapHeaderBase mh(data["info"].str);
  mapheader->updateSymmetryAndCell(mh);
  RecalcResolution();
  return true;
}

bool EMap::PromptForColumnLabels(unsigned int, mmtz_column_*,
                                 int&, int&, int&, int&, int&, int&) {
  // no longer used, as column labels are now set in advance.
  return false;
}

EMap::EMap() : EMapBase() {
  SetVersion(MIFit_version);

  // replace MapSettingsBase with MapSettings (which has GUI load & save support)
  // this will be deleted by EMapBase dtor
  delete settings;
  settings = new MapSettings();
  settings->load();

  // replace CMapHeaderBase with CMapHeader (which has GUI load & save support)
  // this will be deleted by EMapBase dtor
  delete mapheader;
  mapheader = new CMapHeader();
}

bool EMap::FFTMap(int mt, int gl, float rMin, float rMax) {

  if (!HasPhic()) {
    Logger::message("Before you can display the map, you need to use the\n"
      "Calculate Structure Factors... command to\n"
      "calculate phases from a model.");
    return false;
  }

  MIGenericDialog dlg(0,"Fast Fourier Transform Map");
  MIData data;

  std::vector<std::string>& mapTypeOptions = data["mapType"].radio_labels;
  std::vector<unsigned int> maptypes;

  int currentMapType = (mt != -1) ? mt : mapheader->maptype;
  if (currentMapType == (int)MIMapType::DirectFFT) {
    // If DirectFFT, it can only be DirectFFT
    mapTypeOptions.push_back(StringForMapType(MIMapType::DirectFFT));
    maptypes.push_back(MIMapType::DirectFFT);
    data["mapType"].radio=0;
  } else {
    // Limit map types to types which can be calculated given the fields loaded
    std::vector<unsigned int> with_fc_maptypes;
    GetMapTypes(maptypes, with_fc_maptypes);
  
    for (size_t i=0; i < maptypes.size(); ++i) {
      if (maptypes[i] != MIMapType::DirectFFT) {
        // Exclude DirectFFT from list
        mapTypeOptions.push_back(StringForMapType(maptypes[i]));
        if ((int)maptypes[i]==currentMapType) {
          data["mapType"].radio=i;
        }
      }
    }
  }
  data["mapType"].radio_count=mapTypeOptions.size();
  
  data["gridType"].radio=(gl == -1 || gl == -2 ? mapGrid : gl);
  data["gridType"].radio_count=3;
  data["gridType"].radio_labels.push_back("Coarse grid");
  data["gridType"].radio_labels.push_back("Medium grid");
  data["gridType"].radio_labels.push_back("Fine grid");
  
  data["resMin"].f=(rMin == -1.0f ? mapheader->resmin : rMin);
  data["resMax"].f=(rMax == -1.0f ? mapheader->resmax : rMax);
  
  
  dlg.order("resMin");
  dlg.order("resMax");
  dlg.order("mapType");
  dlg.order("gridType");
  dlg.label("resMin","Min resolution:");
  dlg.label("resMax","Max resolution:");
  dlg.label("mapType","Map type:");
  dlg.label("gridType","Grid:");
  
  
  if (mt!=-1 || (gl!=-1 && gl!=-2) || Application::instance()->GetSilentMode() || dlg.GetResults(data))
  {
    bool result = EMapBase::FFTMap(MapTypeForString(StringForMapType(maptypes[data["mapType"].radio])),
                                   (gl==-2?-1:data["gridType"].radio),
                                   data["resMin"].f,
                                   data["resMax"].f);
    mapFftRecalculated(this);
    return result;
  }
  
  return false;
}

void EMap::Export() {
  if (HasPhases()) {
    MIFileDialog fileDialog(NULL, "Choose a name for the phase file", "", "",
                            "CCP4 phase file (*.mtz)|*.mtz|"
                            "mmCIF phase file (*.cif)|*.cif|"
                            "Xtalview phase file (*.phs)|*.phs", MI_SAVE_MODE);
    MIData data;
    data["path"].str = "";
    data["filterIndex"].radio = 0;
    data["filterIndex"].radio_count = 3;
    if (!fileDialog.GetResults(data)) {
      return;
    }

    int choice =  data["filterIndex"].radio;
    std::string path = data["path"].str;
    if (path.size()) {
      int format = CCP4_phase;
      switch (choice) {
        default:
        case 0:
          format = CCP4_phase;
          break;
        case 1:
          format = mmCIF_phase;
          break;
        case 2:
          format = XtalView_phase;
          break;
      }
      SavePhases(path.c_str(), format);
    }
  }
}

int EMap::CenterVisibleEdges(float& x, float& y, float& z, ViewPoint* vp) {
  // look thru the edges, find the on-scrren ones abd return their center
  x = y = z = 0;
  size_t n = 0;
  for (size_t i = 0; i < edges.size(); i++) {
    if (IsOnScreen(edges[i].p1, vp)) {
      x += edges[i].p1.x;
      y += edges[i].p1.y;
      z += edges[i].p1.z;
      n++;
    }
    if (IsOnScreen(edges[i].p2, vp)) {
      x += edges[i].p2.x;
      y += edges[i].p2.y;
      z += edges[i].p2.z;
      n++;
    }
  }
  x = x/(float)n;
  y = y/(float)n;
  z = z/(float)n;
  return n;
}

