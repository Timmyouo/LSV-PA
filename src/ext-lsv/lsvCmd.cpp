#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

ABC_NAMESPACE_IMPL_START

using Lsv_Cut_t = std::vector<int>;

static int Lsv_CommandPrintNodes(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandCutTruthTable(Abc_Frame_t* pAbc, int argc, char** argv);

static void Lsv_CutSort(Lsv_Cut_t& cut);
static Lsv_Cut_t Lsv_CutMerge(const Lsv_Cut_t& cut0, const Lsv_Cut_t& cut1, int k);
static void Lsv_CutPrint(const Lsv_Cut_t& cut);
static void Lsv_CutMergeTest(int k);

static void Lsv_PrintTrivialCuts(Abc_Ntk_t* pNtk);

void init(Abc_Frame_t* pAbc) {
  Cmd_CommandAdd(pAbc, "LSV", "lsv_print_nodes", Lsv_CommandPrintNodes, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_cut_tt", Lsv_CommandCutTruthTable, 0);
}

void destroy(Abc_Frame_t* pAbc) {}

Abc_FrameInitializer_t frame_initializer = {init, destroy};

struct PackageRegistrationManager {
  PackageRegistrationManager() { Abc_FrameAddInitializer(&frame_initializer); }
} lsvPackageRegistrationManager;

static int Lsv_CommandPrintNodes(Abc_Frame_t* pAbc, int argc, char** argv) {
  Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);
  Abc_Obj_t* pObj;
  int i;

  if (!pNtk) {
    Abc_Print(-1, "Empty network.\n");
    return 1;
  }

  Abc_NtkForEachNode(pNtk, pObj, i) {
    printf("Object Id = %d, name = %s\n", Abc_ObjId(pObj), Abc_ObjName(pObj));
  }
  return 0;
}

// Step 4a: merge two cuts into one (union, sort, dedupe, check size <= k).
static void Lsv_CutSort(Lsv_Cut_t& cut) {
  std::sort(cut.begin(), cut.end());
}

static Lsv_Cut_t Lsv_CutMerge(const Lsv_Cut_t& cut0, const Lsv_Cut_t& cut1, int k) {
  Lsv_Cut_t merged = cut0;
  merged.insert(merged.end(), cut1.begin(), cut1.end());
  Lsv_CutSort(merged);
  merged.erase(std::unique(merged.begin(), merged.end()), merged.end());

  if ((int)merged.size() > k) {
    return {};
  }
  return merged;
}

static void Lsv_CutPrint(const Lsv_Cut_t& cut) {
  printf("{");
  for (size_t i = 0; i < cut.size(); ++i) {
    printf("%s%d", (i == 0) ? "" : ",", cut[i]);
  }
  printf("}");
}

// Temporary sanity check for Step 4a; remove after Step 4c.
static void Lsv_CutMergeTest(int k) {
  Lsv_Cut_t result;

  result = Lsv_CutMerge({1}, {2}, k);
  printf("merge {1} + {2}, k=%d -> ", k);
  Lsv_CutPrint(result);
  printf("\n");

  result = Lsv_CutMerge({1, 2}, {2, 3}, k);
  printf("merge {1,2} + {2,3}, k=%d -> ", k);
  Lsv_CutPrint(result);
  printf("\n");

  result = Lsv_CutMerge({1, 2}, {3, 4}, k);
  printf("merge {1,2} + {3,4}, k=%d -> ", k);
  if (result.empty()) {
    printf("(invalid, size > k)\n");
  } else {
    Lsv_CutPrint(result);
    printf("\n");
  }
}

// Step 3: print trivial cut {n} for each internal AND node.
static void Lsv_PrintTrivialCuts(Abc_Ntk_t* pNtk) {
  Abc_Obj_t* pObj;
  int i;

  Abc_AigForEachAnd(pNtk, pObj, i) {
    const int nodeId = Abc_ObjId(pObj);
    printf("%d: %d\n", nodeId, nodeId);
  }
}

static int Lsv_CommandCutTruthTable(Abc_Frame_t* pAbc, int argc, char** argv) {
  Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);
  int k;

  if (!pNtk) {
    Abc_Print(-1, "Empty network.\n");
    return 1;
  }
  if (!Abc_NtkIsStrash(pNtk)) {
    Abc_Print(-1, "This command works only for AIGs (run \"strash\").\n");
    return 1;
  }
  if (argc < 2) {
    Abc_Print(-1, "Missing cut size parameter <k>.\n");
    Abc_Print(-2, "usage: lsv_cut_tt <k>\n");
    return 1;
  }

  k = atoi(argv[1]);
  if (k < 2 || k > 6) {
    Abc_Print(-1, "Invalid cut size k = %d (expected 2 <= k <= 6).\n", k);
    return 1;
  }

  Lsv_CutMergeTest(k);
  Lsv_PrintTrivialCuts(pNtk);
  return 0;
}

ABC_NAMESPACE_IMPL_END
