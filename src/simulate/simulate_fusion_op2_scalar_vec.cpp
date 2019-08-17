#include "daScript/misc/platform.h"

#ifdef _MSC_VER
#pragma warning(disable:4505)
#endif

#include "daScript/ast/ast.h"
#include "daScript/simulate/simulate_fusion.h"
#include "daScript/simulate/sim_policy.h"
#include "daScript/simulate/simulate_nodes.h"
#include "daScript/simulate/simulate_visit_op.h"

// fake DAS_NODE to support regular eval
#undef DAS_NODE
#define DAS_NODE(TYPE,CTYPE)                                    \
    virtual vec4f eval ( das::Context & context ) override {    \
        return compute(context);                                \
    }

#include "daScript/simulate/simulate_fusion_vec_settings.h"

#ifdef FUSION_OP_PTR_VALUE
#undef FUSION_OP_PTR_VALUE
#endif

#define FUSION_OP_PTR_VALUE(CTYPE,expr)    *((CTYPE *)((expr)))

#include "daScript/simulate/simulate_fusion_op2.h"

namespace das {
    IMPLEMENT_OP2_VEC_NUMERIC_FUSION_POINT(MulVecScal);
    IMPLEMENT_OP2_VEC_NUMERIC_FUSION_POINT(DivVecScal);
    IMPLEMENT_OP2_VEC_NUMERIC_FUSION_POINT(MulScalVec);
    IMPLEMENT_OP2_VEC_NUMERIC_FUSION_POINT(DivScalVec);

    void createFusionEngine_op2_scalar_vec()
    {
        REGISTER_OP2_VEC_NUMERIC_FUSION_POINT(MulVecScal);
        REGISTER_OP2_VEC_NUMERIC_FUSION_POINT(DivVecScal);
        REGISTER_OP2_VEC_NUMERIC_FUSION_POINT(MulScalVec);
        REGISTER_OP2_VEC_NUMERIC_FUSION_POINT(DivScalVec);
    }
}


