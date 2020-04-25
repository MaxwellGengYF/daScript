#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    // AOT

    AotListBase * AotListBase::head = nullptr;

    AotListBase::AotListBase() {
        tail = head;
        head = this;
    }

    void AotListBase::registerAot ( AotLibrary & lib ) {
        auto it = head;
        while ( it ) {
            it->registerAotFunctions(lib);
            it = it->tail;
        }
    }

    // annotations

    string Annotation::getMangledName() const {
        return module ? module->name + "::" + name : name;
    }

    string FunctionAnnotation::aotName ( ExprCallFunc * call ) {
        return call->func->getAotBasicName();
    }

    string AnnotationDeclaration::getMangledName() const {
        TextWriter tw;
        tw << "[" << annotation->getMangledName();
        if (arguments.size()) {
            tw << "(";
            for (auto & arg : arguments) {
                if (&arg != &arguments.front()) {
                    tw << ",";
                }
                tw << arg.name << ":" << das_to_string(arg.type) << "=";
                switch (arg.type) {
                case Type::tBool:       tw << (arg.bValue ? "true" : "false"); break;
                case Type::tInt:        tw << arg.iValue; break;
                case Type::tFloat:      tw << arg.fValue; break;
                case Type::tString:     tw << "\"" << arg.sValue << "\""; break;
                default:                tw << "error"; break;
                }
            }
            tw << ")";
        }
        tw << "]";
        return tw.str();
    }

    // enumeration

    string Enumeration::getMangledName() const {
        return (module ? module->name+"::"+name : name) + "#" + das_to_string(baseType);
    }

    TypeDeclPtr Enumeration::makeBaseType() const { 
        return make_smart<TypeDecl>(baseType); 
    }

    Type Enumeration::getEnumType() const {
        switch (baseType) {
        case Type::tInt8:
        case Type::tUInt8:
            return Type::tEnumeration8;
        case Type::tInt16:
        case Type::tUInt16:
            return Type::tEnumeration16;
        case Type::tInt:
        case Type::tUInt:
            return Type::tEnumeration;
        default:
            DAS_ASSERTF(0, "we should not be here. unsupported enumeration base type.");
            return Type::none;
        }
    }

    TypeDeclPtr Enumeration::makeEnumType() const {
        TypeDeclPtr res;
        switch (baseType) {
        case Type::tInt8:
        case Type::tUInt8:
            res = make_smart<TypeDecl>(Type::tEnumeration8); break;
        case Type::tInt16:
        case Type::tUInt16:
            res = make_smart<TypeDecl>(Type::tEnumeration16); break;
        case Type::tInt:
        case Type::tUInt:
            res = make_smart<TypeDecl>(Type::tEnumeration); break;
        default:
            DAS_ASSERTF(0, "we should not be here. unsupported enumeration base type.");
            return nullptr;
        }
        res->enumType = const_cast<Enumeration*>(this);
        return res;
    }


    pair<ExpressionPtr,bool> Enumeration::find ( const string & na ) const {
        auto it = find_if(list.begin(), list.end(), [&](const pair<string,ExpressionPtr> & arg){
            return arg.first == na;
        });
        return it!=list.end() ?
            pair<ExpressionPtr,bool>(it->second,true) :
            pair<ExpressionPtr,bool>(nullptr,false);
    }

    int64_t Enumeration::find ( const string & na, int64_t def ) const {
        auto it = find_if(list.begin(), list.end(), [&](const pair<string,ExpressionPtr> & arg){
            return arg.first == na;
        });
        return it!=list.end() ? getConstExprIntOrUInt(it->second) : def;
    }

    string Enumeration::find ( int64_t va, const string & def ) const {
        for ( const auto & it : list ) {
            if ( getConstExprIntOrUInt(it.second)==va ) {
                return it.first;
            }
        }
        return def;
    }

    bool Enumeration::add ( const string & f ) {
        return add(f, nullptr);
    }

    bool Enumeration::addI ( const string & f, int64_t value ) {
        return add(f, make_smart<ExprConstInt64>(value));
    }

    bool Enumeration::add ( const string & na, const ExpressionPtr & expr ) {
        auto it = find_if(list.begin(), list.end(), [&](const pair<string,ExpressionPtr> & arg){
            return arg.first == na;
        });
        if ( it == list.end() ) {
            list.emplace_back(na, expr);
            return true;
        } else {
            return false;
        }
    }

    // structure

    StructurePtr Structure::clone() const {
        auto cs = make_smart<Structure>(name);
        cs->fields.reserve(fields.size());
        for ( auto & fd : fields ) {
            cs->fields.emplace_back(fd.name, fd.type, fd.init, fd.annotation, fd.moveSemantic, fd.at);
        }
        cs->at = at;
        cs->module = module;
        cs->genCtor = genCtor;
        cs->cppLayout = cppLayout;
        cs->cppLayoutPod = cppLayoutPod;
        cs->annotations = annotations;
        return cs;
    }

    bool Structure::isCompatibleCast ( const Structure & castS ) const {
        if ( castS.fields.size() < fields.size() ) {
            return false;
        }
        for ( size_t i=0; i!=fields.size(); ++i ) {
            auto & fd = fields[i];
            auto & cfd = castS.fields[i];
            if ( fd.name != cfd.name ) {
                return false;
            }
            if ( !fd.type->isSameType(*cfd.type, RefMatters::yes, ConstMatters::yes, TemporaryMatters::yes) ) {
                return false;
            }
        }
        return true;
    }

    bool Structure::hasAnyInitializers() const {
        for ( const auto & fd : fields ) {
            if ( fd.init ) return true;
        }
        return false;
    }

    bool Structure::canCopy() const {
        for ( const auto & fd : fields ) {
            if ( !fd.type->canCopy() )
                return false;
        }
        return true;
    }

    bool Structure::canMove() const {
        for ( const auto & fd : fields ) {
            if ( !fd.type->canMove() )
                return false;
        }
        return true;
    }

    bool Structure::canClone() const {
        for ( const auto & fd : fields ) {
            if ( !fd.type->canClone() )
                return false;
        }
        return true;
    }

    bool Structure::canAot( das_set<Structure *> & recAot ) const {
        for ( const auto & fd : fields ) {
            if ( !fd.type->canAot(recAot) )
                return false;
        }
        return true;
    }

    bool Structure::canAot() const {
        das_set<Structure *> recAot;
        return canAot(recAot);
    }

    bool Structure::isNoHeapType() const {
        for ( const auto & fd : fields ) {
            if ( !fd.type->isNoHeapType() )
                return false;
        }
        return true;
    }

    bool Structure::isPod() const {
        for ( const auto & fd : fields ) {
            if ( !fd.type->isPod() )
                return false;
        }
        return true;
    }

    bool Structure::isRawPod() const {
        for ( const auto & fd : fields ) {
            if ( !fd.type->isRawPod() )
                return false;
        }
        return true;
    }

    int Structure::getSizeOf() const {
        int size = 0;
        const Structure * cppLayoutParent = nullptr;
        for ( const auto & fd : fields ) {
            int fieldAlignemnt = fd.type->getAlignOf();
            int al = fieldAlignemnt - 1;
            if ( cppLayout ) {
                auto fp = findFieldParent(fd.name);
                if ( fp!=cppLayoutParent ) {
                    if (DAS_NON_POD_PADDING || cppLayoutPod) {
                        size = cppLayoutParent ? cppLayoutParent->getSizeOf() : 0;
                    }
                    cppLayoutParent = fp;
                }
            }
            size = (size + al) & ~al;
            size += fd.type->getSizeOf();
        }
        int al = getAlignOf() - 1;
        size = (size + al) & ~al;
        return size;
    }

    int Structure::getAlignOf() const {
        int align = 1;
        for ( const auto & fd : fields ) {
            align = das::max ( fd.type->getAlignOf(), align );
        }
        return align;
    }

    const Structure::FieldDeclaration * Structure::findField ( const string & na ) const {
        for ( const auto & fd : fields ) {
            if ( fd.name==na ) {
                return &fd;
            }
        }
        return nullptr;
    }

    const Structure * Structure::findFieldParent ( const string & na ) const {
        if ( parent ) {
            if ( parent->findField(na) ) {
                return parent;
            }
        }
        if ( findField(na) ) {
            return this;
        }
        return nullptr;
    }

    string Structure::getMangledName() const {
        return module ? module->name+"::"+name : name;
    }

    bool Structure::isLocal(das_set<Structure *> & dep) const {   // &&
        for ( const auto & fd : fields ) {
            if ( fd.type && !fd.type->isLocal(dep) ) {
                return false;
            }
        }
        return true;
    }

    bool Structure::isTemp(das_set<Structure *> & dep) const {    // ||
        for ( const auto & fd : fields ) {
            if ( fd.type && fd.type->isTemp(true, true, dep) ) {
                return true;
            }
        }
        return false;
    }

    bool Structure::isShareable(das_set<Structure *> & dep) const {    // &&
        for ( const auto & fd : fields ) {
            if ( fd.type && !fd.type->isShareable(dep) ) {
                return false;
            }
        }
        return true;
    }

    // variable

    VariablePtr Variable::clone() const {
        auto pVar = make_smart<Variable>();
        pVar->name = name;
        pVar->type = make_smart<TypeDecl>(*type);
        if ( init )
            pVar->init = init->clone();
        if ( source )
            pVar->source = source->clone();
        pVar->at = at;
        pVar->flags = flags;
        pVar->initStackSize = initStackSize;
        pVar->annotation = annotation;
        return pVar;
    }

    string Variable::getMangledName() const {
        string mn = module ? module->name+"::"+name : name;
        return mn + " " + type->getMangledName();
    }

    bool Variable::isAccessUnused() const {
        return !(access_get || access_init || access_pass || access_ref);
    }

    // function

    FunctionPtr Function::clone() const {
        auto cfun = make_smart<Function>();
        cfun->name = name;
        for ( const auto & arg : arguments ) {
            cfun->arguments.push_back(arg->clone());
        }
        cfun->annotations = annotations;
        cfun->result = make_smart<TypeDecl>(*result);
        cfun->body = body->clone();
        cfun->index = -1;
        cfun->totalStackSize = 0;
        cfun->totalGenLabel = totalGenLabel;
        cfun->at = at;
        cfun->module = nullptr;
        cfun->flags = flags;
        cfun->sideEffectFlags = sideEffectFlags;
        cfun->inferStack = inferStack;
        return cfun;
    }

    LineInfo Function::getConceptLocation(const LineInfo & atL) const {
        return inferStack.size() ? inferStack[0].at : atL;
    }

    string Function::getLocationExtra() const {
        if ( !inferStack.size() ) {
            return "";
        }
        TextWriter ss;
        ss << "\nwhile compiling " << describe() << "\n";
        for ( const auto & ih : inferStack ) {
            ss << "instanced from " << ih.func->describe() << " at " << ih.at.describe() << "\n";
        }
        return ss.str();
    }

    string Function::describe() const {
        TextWriter ss;
        if ( !isalpha(name[0]) && name[0]!='_' && name[0]!='`' ) {
            ss << "operator ";
        }
        ss << name;
        if ( arguments.size() ) {
            ss << " ( ";
            for ( auto & arg : arguments ) {
                ss << arg->name << " : " << *arg->type;
                if ( arg != arguments.back() ) ss << "; ";
            }
            ss << " )";
        }
        ss << " : " << result->describe();
        return ss.str();
    }

    string Function::getMangledName() const {
        TextWriter ss;
        if ( module && !module->name.empty() ) {
            ss << "@" << module->name << "::";
        }
        ss << name;
        for ( auto & arg : arguments ) {
            ss << " " << arg->type->getMangledName();
        }
        return ss.str();
    }

    VariablePtr Function::findArgument(const string & na) {
        for ( auto & arg : arguments ) {
            if ( arg->name==na ) {
                return arg;
            }
        }
        return nullptr;
    }

    FunctionPtr Function::visit(Visitor & vis) {
        vis.preVisit(this);
        for ( auto & arg : arguments ) {
            vis.preVisitArgument(this, arg, arg==arguments.back() );
            if ( arg->type ) {
                vis.preVisit(arg->type.get());
                arg->type = arg->type->visit(vis);
                arg->type = vis.visit(arg->type.get());
            }
            if ( arg->init ) {
                vis.preVisitArgumentInit(this, arg, arg->init.get());
                arg->init = arg->init->visit(vis);
                if ( arg->init ) {
                    arg->init = vis.visitArgumentInit(this, arg, arg->init.get());
                }
            }
            arg = vis.visitArgument(this, arg, arg==arguments.back() );
        }
        if ( result ) {
            vis.preVisit(result.get());
            result = result->visit(vis);
            result = vis.visit(result.get());
        }
        vis.preVisitFunctionBody(this, body.get());
        body = body->visit(vis);
        body = vis.visitFunctionBody(this, body.get());
        return vis.visit(this);
    }

    bool Function::isGeneric() const {
        for ( const auto & ann : annotations ) {
            if (ann->annotation) {
                auto fna = static_pointer_cast<FunctionAnnotation>(ann->annotation);
                if (fna->isGeneric()) {
                    return true;
                }
            }
        }
        for ( auto & arg : arguments ) {
            if ( arg->type->isAuto() && !arg->init ) {
                return true;
            }
        }
        return false;
    }

    string Function::getAotName(ExprCallFunc * call) const {
        for ( auto & ann : annotations ) {
            if ( ann->annotation->rtti_isFunctionAnnotation() ) {
                auto pAnn = static_pointer_cast<FunctionAnnotation>(ann->annotation);
                return pAnn->aotName(call);
            }
        }
        return call->func->getAotBasicName();
    }

    FunctionPtr Function::setSideEffects ( SideEffects seFlags ) {
        sideEffectFlags = uint32_t(seFlags) & ~uint32_t(SideEffects::unsafe);
        if ( uint32_t(seFlags) & uint32_t(SideEffects::unsafe) ) {
            unsafeOperation = true;
        }
        return this;
    }

    FunctionPtr Function::setAotTemplate() {
        aotTemplate = true;
        return this;
    }

    // built-in function

    BuiltInFunction::BuiltInFunction ( const string & fn, const string & fnCpp ) {
        builtIn = true;
        name = fn;
        cppName = fnCpp;
    }

    // expression

    ExpressionPtr Expression::clone( const ExpressionPtr & expr ) const {
        if ( !expr ) {
            DAS_ASSERTF(0,
                   "its not ok to clone Expression as is."
                   "if we are here, this means that ::clone function is not written correctly."
                   "incorrect clone function can be found on the stack above this.");
            return nullptr;
        }
        expr->at = at;
        expr->type = type ? make_smart<TypeDecl>(*type) : nullptr;
        expr->genFlags = genFlags;
        return expr;
    }

    ExpressionPtr Expression::autoDereference ( const ExpressionPtr & expr ) {
        if ( expr->type && !expr->type->isAuto() && expr->type->isRef() && !expr->type->isRefType() ) {
            auto ar2l = make_smart<ExprRef2Value>();
            ar2l->subexpr = expr;
            ar2l->at = expr->at;
            ar2l->type = make_smart<TypeDecl>(*expr->type);
            ar2l->type->ref = false;
            return ar2l;
        } else {
            return expr;
        }
    }

    // Label

    ExpressionPtr ExprLabel::visit(Visitor & vis) {
        vis.preVisit(this);
        return vis.visit(this);
    }

    ExpressionPtr ExprLabel::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprLabel>(expr);
        Expression::clone(cexpr);
        cexpr->label = label;
        cexpr->comment = comment;
        return cexpr;
    }

    // Goto

    ExpressionPtr ExprGoto::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( subexpr ) {
            subexpr = subexpr->visit(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprGoto::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprGoto>(expr);
        Expression::clone(cexpr);
        cexpr->label = label;
        if ( subexpr ) {
            cexpr->subexpr = subexpr->clone();
        }
        return cexpr;
    }

    // ExprRef2Value

    ExpressionPtr ExprRef2Value::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprRef2Value::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprRef2Value>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        return cexpr;
    }

    // ExprRef2Ptr

    ExpressionPtr ExprRef2Ptr::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprRef2Ptr::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprRef2Ptr>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        return cexpr;
    }

    // ExprPtr2Ref

    ExpressionPtr ExprPtr2Ref::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprPtr2Ref::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprPtr2Ref>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        return cexpr;
    }

    // ExprAddr

    ExpressionPtr ExprAddr::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( funcType ) {
            vis.preVisit(funcType.get());
            funcType = funcType->visit(vis);
            funcType = vis.visit(funcType.get());
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprAddr::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprAddr>(expr);
        Expression::clone(cexpr);
        cexpr->target = target;
        cexpr->func = func;
        if (funcType) cexpr->funcType = make_smart<TypeDecl>(*funcType);
        return cexpr;
    }

    // ExprNullCoalescing

    ExpressionPtr ExprNullCoalescing::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        vis.preVisitNullCoaelescingDefault(this, defaultValue.get());
        defaultValue = defaultValue->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprNullCoalescing::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprNullCoalescing>(expr);
        ExprPtr2Ref::clone(cexpr);
        cexpr->defaultValue = defaultValue->clone();
        return cexpr;
    }

    // ConstEnumeration

    ExpressionPtr ExprConstEnumeration::visit(Visitor & vis) {
        vis.preVisit((ExprConst *)this);
        vis.preVisit(this);
        auto res = vis.visit(this);
        if ( res.get() != this )
            return res;
        return vis.visit((ExprConst *)this);
    }

    ExpressionPtr ExprConstEnumeration::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprConstEnumeration>(expr);
        cexpr->enumType = enumType;
        cexpr->text = text;
        return cexpr;
    }

    // ConstString

    ExpressionPtr ExprConstString::visit(Visitor & vis) {
        vis.preVisit((ExprConst *)this);
        vis.preVisit(this);
        auto res = vis.visit(this);
        if ( res.get() != this )
            return res;
        return vis.visit((ExprConst *)this);
    }

    ExpressionPtr ExprConstString::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprConstString>(expr);
        Expression::clone(cexpr);
        cexpr->value = value;
        cexpr->text = text;
        return cexpr;
    }

    // ExprStaticAssert

    ExpressionPtr ExprStaticAssert::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprStaticAssert>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprAssert

    ExpressionPtr ExprAssert::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprAssert>(expr);
        ExprLooksLikeCall::clone(cexpr);
        cexpr->isVerify = isVerify;
        return cexpr;
    }

    // ExprDebug

    ExpressionPtr ExprDebug::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprDebug>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprMemZero

    ExpressionPtr ExprMemZero::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprMemZero>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprMakeGenerator

    ExprMakeGenerator::ExprMakeGenerator ( const LineInfo & a, const ExpressionPtr & b )
    : ExprLooksLikeCall(a, "generator") {
        if ( b ) {
            arguments.push_back(b);
        }
    }

    ExpressionPtr ExprMakeGenerator::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( iterType ) {
            vis.preVisit(iterType.get());
            iterType = iterType->visit(vis);
            iterType = vis.visit(iterType.get());
        }
        ExprLooksLikeCall::visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprMakeGenerator::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprMakeGenerator>(expr);
        ExprLooksLikeCall::clone(cexpr);
        if ( iterType ) {
            cexpr->iterType = make_smart<TypeDecl>(*iterType);
        }
        return cexpr;
    }

    // ExprYield

    ExprYield::ExprYield ( const LineInfo & a, const ExpressionPtr & b ) : Expression(a) {
        subexpr = b;
    }

    ExpressionPtr ExprYield::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprYield::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprYield>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->returnFlags = returnFlags;
        return cexpr;
    }

    // ExprMakeBlock

    ExpressionPtr ExprMakeBlock::visit(Visitor & vis) {
        vis.preVisit(this);
        block = block->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprMakeBlock::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprMakeBlock>(expr);
        Expression::clone(cexpr);
        cexpr->block = block->clone();
        cexpr->isLambda = isLambda;
        return cexpr;
    }

    // ExprInvoke

    ExpressionPtr ExprInvoke::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprInvoke>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprErase

    ExpressionPtr ExprErase::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprErase>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprFind

    ExpressionPtr ExprFind::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprFind>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprKeyExists

    ExpressionPtr ExprKeyExists::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprKeyExists>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprIs

    ExpressionPtr ExprIs::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        vis.preVisitType(this, typeexpr.get());
        vis.preVisit(typeexpr.get());
        typeexpr = typeexpr->visit(vis);
        typeexpr = vis.visit(typeexpr.get());
        return vis.visit(this);
    }

    ExpressionPtr ExprIs::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprTypeInfo>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->typeexpr = make_smart<TypeDecl>(*typeexpr);
        return cexpr;
    }

    // ExprTypeInfo

    ExpressionPtr ExprTypeInfo::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( typeexpr ) {
            vis.preVisit(typeexpr.get());
            typeexpr = typeexpr->visit(vis);
            typeexpr = vis.visit(typeexpr.get());
        }
        if ( subexpr ) {
            subexpr = subexpr->visit(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprTypeInfo::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprTypeInfo>(expr);
        Expression::clone(cexpr);
        cexpr->trait = trait;
        cexpr->subtrait = subtrait;
        cexpr->extratrait = extratrait;
        if ( subexpr )
            cexpr->subexpr = subexpr->clone();
        if ( typeexpr )
            cexpr->typeexpr = typeexpr;
        return cexpr;
    }

    // ExprDelete

    ExpressionPtr ExprDelete::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( subexpr ) {
            subexpr = subexpr->visit(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprDelete::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprDelete>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->native = native;
        return cexpr;
    }

    // ExprCast

    ExpressionPtr ExprCast::clone( const ExpressionPtr & expr  ) const {
        auto cexpr = clonePtr<ExprCast>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->castType = make_smart<TypeDecl>(*castType);
        cexpr->castFlags = castFlags;
        return cexpr;
    }

    ExpressionPtr ExprCast::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( castType ) {
            vis.preVisit(castType.get());
            castType = castType->visit(vis);
            castType = vis.visit(castType.get());
        }
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }

    // ExprAscend

    ExpressionPtr ExprAscend::clone( const ExpressionPtr & expr  ) const {
        auto cexpr = clonePtr<ExprAscend>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->ascendFlags = ascendFlags;
        return cexpr;
    }

    ExpressionPtr ExprAscend::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }

    // ExprNew

    ExpressionPtr ExprNew::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( typeexpr ) {
            vis.preVisit(typeexpr.get());
            typeexpr = typeexpr->visit(vis);
            typeexpr = vis.visit(typeexpr.get());
        }
        for ( auto & arg : arguments ) {
            vis.preVisitNewArg(this, arg.get(), arg==arguments.back());
            arg = arg->visit(vis);
            arg = vis.visitNewArg(this, arg.get(), arg==arguments.back());
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprNew::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprNew>(expr);
        ExprLooksLikeCall::clone(cexpr);
        cexpr->typeexpr = typeexpr;
        cexpr->initializer = initializer;
        return cexpr;
    }

    // ExprAt

    ExpressionPtr ExprAt::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        vis.preVisitAtIndex(this, index.get());
        index = index->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprAt::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprAt>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->index = index->clone();
        return cexpr;
    }

    // ExprSafeAt

    ExpressionPtr ExprSafeAt::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        vis.preVisitSafeAtIndex(this, index.get());
        index = index->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprSafeAt::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprSafeAt>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->index = index->clone();
        return cexpr;
    }

    // ExprBlock

    bool ExprBlock::collapse() {
        bool any = false;
        if ( !list.empty() ) {
            vector<ExpressionPtr> lst;
            collapse(lst, list);
            if ( lst != list ) {
                swap ( list, lst );
                any = true;
            }
        }
        if ( !finalList.empty() ) {
            vector<ExpressionPtr> flst;
            collapse(flst, finalList);
            if ( flst != finalList ) {
                swap ( finalList, flst );
                any = true;
            }
        }
        return any;
    }

    void ExprBlock::collapse ( vector<ExpressionPtr> & res, const vector<ExpressionPtr> & lst ) {
        for ( const auto & ex :lst ) {
            if ( ex->rtti_isBlock() ) {
                auto blk = static_pointer_cast<ExprBlock>(ex);
                if ( blk->isCollapseable && blk->finalList.empty() ) {
                    collapse(res, blk->list);
                } else {
                    res.push_back(ex);
                }
            } else {
                res.push_back(ex);
            }
        }
    }

    string ExprBlock::getMangledName(bool includeName, bool includeResult) const {
        TextWriter ss;
        if ( includeResult ) {
            ss << returnType->getMangledName();
        }
        for ( auto & arg : arguments ) {
            if ( includeName ) {
                ss << " " << arg->name << ":" << arg->type->getMangledName();
            } else {
                ss << " " << arg->type->getMangledName();
            }
        }
        return ss.str();
    }

    TypeDeclPtr ExprBlock::makeBlockType () const {
        auto eT = make_smart<TypeDecl>(Type::tBlock);
        eT->constant = true;
        if ( type ) {
            eT->firstType = make_smart<TypeDecl>(*type);
        }
        for ( auto & arg : arguments ) {
            if ( arg->type ) {
                eT->argTypes.push_back(make_smart<TypeDecl>(*arg->type));
            }
        }
        return eT;
    }

    void ExprBlock::visitFinally(Visitor & vis) {
        if ( !finalList.empty() && !finallyDisabled ) {
            vis.preVisitBlockFinal(this);
            for ( auto it = finalList.begin(); it!=finalList.end(); ) {
                auto & subexpr = *it;
                vis.preVisitBlockFinalExpression(this, subexpr.get());
                subexpr = subexpr->visit(vis);
                if ( subexpr )
                    subexpr = vis.visitBlockFinalExpression(this, subexpr.get());
                if ( subexpr ) ++it; else it = finalList.erase(it);
            }
            vis.visitBlockFinal(this);
        }
    }

    ExpressionPtr ExprBlock::visit(Visitor & vis) {
        vis.preVisit(this);
        for ( auto it = arguments.begin(); it != arguments.end(); ) {
            auto & arg = *it;
            vis.preVisitBlockArgument(this, arg, arg==arguments.back());
            if ( arg->type ) {
                vis.preVisit(arg->type.get());
                arg->type = arg->type->visit(vis);
                arg->type = vis.visit(arg->type.get());
            }
            if ( arg->init ) {
                vis.preVisitBlockArgumentInit(this, arg, arg->init.get());
                arg->init = arg->init->visit(vis);
                if ( arg->init ) {
                    arg->init = vis.visitBlockArgumentInit(this, arg, arg->init.get());
                }
            }
            arg = vis.visitBlockArgument(this, arg, arg==arguments.back());
            if ( arg ) ++it; else it = arguments.erase(it);
        }
        if ( finallyBeforeBody ) {
            visitFinally(vis);
        }
        for ( auto it = list.begin(); it!=list.end(); ) {
            auto & subexpr = *it;
            vis.preVisitBlockExpression(this, subexpr.get());
            subexpr = subexpr->visit(vis);
            if ( subexpr )
                subexpr = vis.visitBlockExpression(this, subexpr.get());
            if ( subexpr ) ++it; else it = list.erase(it);
        }
        if ( !finallyBeforeBody ) {
            visitFinally(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprBlock::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprBlock>(expr);
        Expression::clone(cexpr);
        cexpr->list.reserve(list.size());
        for ( auto & subexpr : list ) {
            cexpr->list.push_back(subexpr->clone());
        }
        cexpr->finalList.reserve(finalList.size());
        for ( auto & subexpr : finalList ) {
            cexpr->finalList.push_back(subexpr->clone());
        }
        cexpr->blockFlags = blockFlags;
        if ( returnType )
            cexpr->returnType = make_smart<TypeDecl>(*returnType);
        for ( auto & arg : arguments ) {
            cexpr->arguments.push_back(arg->clone());
        }
        cexpr->annotations = annotations;
        cexpr->maxLabelIndex = maxLabelIndex;
        return cexpr;
    }

    uint32_t ExprBlock::getEvalFlags() const {
        uint32_t flg = getFinallyEvalFlags();
        for ( const auto & ex : list ) {
            flg |= ex->getEvalFlags();
        }
        return flg;
    }

    uint32_t ExprBlock::getFinallyEvalFlags() const {
        uint32_t flg = 0;
        for ( const auto & ex : finalList ) {
            flg |= ex->getEvalFlags();
        }
        return flg;
    }

    VariablePtr ExprBlock::findArgument(const string & name) {
        for ( auto & arg : arguments ) {
            if ( arg->name==name ) {
                return arg;
            }
        }
        return nullptr;
    }

    // ExprSwizzle

    ExpressionPtr ExprSwizzle::visit(Visitor & vis) {
        vis.preVisit(this);
        value = value->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprSwizzle::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprSwizzle>(expr);
        Expression::clone(cexpr);
        cexpr->mask = mask;
        cexpr->value = value->clone();
        return cexpr;
    }

    // ExprField

    ExpressionPtr ExprField::visit(Visitor & vis) {
        vis.preVisit(this);
        value = value->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprField::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprField>(expr);
        Expression::clone(cexpr);
        cexpr->name = name;
        cexpr->value = value->clone();
        cexpr->field = field;
        cexpr->tupleOrVariantIndex = tupleOrVariantIndex;
        cexpr->unsafeDeref = unsafeDeref;
        return cexpr;
    }

    int ExprField::tupleFieldIndex() const {
        int index = 0;
        if ( sscanf(name.c_str(),"_%i",&index)==1 ) {
            return index;
        } else {
            auto vT = value->type->isPointer() ? value->type->firstType : value->type;
            if (!vT) return -1;
            if ( name=="_first" ) {
                return 0;
            } else if ( name=="_last" ) {
                return int(vT->argTypes.size())-1;
            } else {
                return vT->findArgumentIndex(name);
            }
            return -1;
        }
    }

    int ExprField::variantFieldIndex() const {
        auto vT = value->type->isPointer() ? value->type->firstType : value->type;
        if (!vT) return -1;
        return vT->findArgumentIndex(name);
    }

    // ExprIs

    ExpressionPtr ExprIsVariant::visit(Visitor & vis) {
        vis.preVisit(this);
        value = value->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprIsVariant::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprIsVariant>(expr);
        ExprField::clone(cexpr);
        return cexpr;
    }

    // ExprAsVariant

    ExpressionPtr ExprAsVariant::visit(Visitor & vis) {
        vis.preVisit(this);
        value = value->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprAsVariant::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprAsVariant>(expr);
        ExprField::clone(cexpr);
        return cexpr;
    }

    // ExprSafeAsVariant

    ExpressionPtr ExprSafeAsVariant::visit(Visitor & vis) {
        vis.preVisit(this);
        value = value->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprSafeAsVariant::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprSafeAsVariant>(expr);
        ExprField::clone(cexpr);
        return cexpr;
    }

    // ExprSafeField

    ExpressionPtr ExprSafeField::visit(Visitor & vis) {
        vis.preVisit(this);
        value = value->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprSafeField::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprSafeField>(expr);
        ExprField::clone(cexpr);
        return cexpr;
    }

    // ExprStringBuilder

    ExpressionPtr ExprStringBuilder::visit(Visitor & vis) {
        vis.preVisit(this);
        for ( auto it = elements.begin(); it!=elements.end(); ) {
            auto & elem = *it;
            vis.preVisitStringBuilderElement(this, elem.get(), elem==elements.back());
            elem = elem->visit(vis);
            if ( elem ) elem = vis.visitStringBuilderElement(this, elem.get(), elem==elements.back());
            if ( elem ) ++ it; else it = elements.erase(it);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprStringBuilder::clone( const ExpressionPtr & expr  ) const {
        auto cexpr = clonePtr<ExprStringBuilder>(expr);
        Expression::clone(cexpr);
        cexpr->elements.reserve(elements.size());
        for ( auto & elem : elements ) {
            cexpr->elements.push_back(elem->clone());
        }
        return cexpr;
    }

    // ExprVar

    ExpressionPtr ExprVar::visit(Visitor & vis) {
        vis.preVisit(this);
        return vis.visit(this);
    }

    ExpressionPtr ExprVar::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprVar>(expr);
        Expression::clone(cexpr);
        cexpr->name = name;
        cexpr->variable = variable; 
        cexpr->local = local;
        cexpr->block = block;
        cexpr->pBlock = pBlock;
        cexpr->argument = argument;
        cexpr->argumentIndex = argumentIndex;
        return cexpr;
    }

    // ExprOp

    ExpressionPtr ExprOp::clone( const ExpressionPtr & expr ) const {
        if ( !expr ) {
            DAS_ASSERTF(0,"can't clone ExprOp");
            return nullptr;
        }
        auto cexpr = static_pointer_cast<ExprOp>(expr);
        ExprCallFunc::clone(cexpr);
        cexpr->op = op;
        cexpr->func = func;
        cexpr->at = at;
        return cexpr;
    }

    // ExprOp1

    ExpressionPtr ExprOp1::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprOp1::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprOp1>(expr);
        ExprOp::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        return cexpr;
    }

    string ExprOp1::describe() const {
        TextWriter stream;
        stream << name << " ( ";
        if ( subexpr && subexpr->type ) {
            stream << *subexpr->type;
        } else {
            stream << "???";
        }
        stream << " )";
        return stream.str();
    }

    // ExprOp2

    ExpressionPtr ExprOp2::visit(Visitor & vis) {
        vis.preVisit(this);
        left = left->visit(vis);
        vis.preVisitRight(this, right.get());
        right = right->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprOp2::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprOp2>(expr);
        ExprOp::clone(cexpr);
        cexpr->left = left->clone();
        cexpr->right = right->clone();
        return cexpr;
    }

    string ExprOp2::describe() const {
        TextWriter stream;
        stream << name << " ( ";
        if ( left && left->type ) {
            stream << *left->type;
        } else {
            stream << "???";
        }
        stream << ", ";
        if ( right && right->type ) {
            stream << *right->type;
        } else {
            stream << "???";
        }
        stream << " )";
        return stream.str();
    }

    // ExprOp3

    ExpressionPtr ExprOp3::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        vis.preVisitLeft(this, left.get());
        left = left->visit(vis);
        vis.preVisitRight(this, right.get());
        right = right->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprOp3::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprOp3>(expr);
        ExprOp::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->left = left->clone();
        cexpr->right = right->clone();
        return cexpr;
    }

    string ExprOp3::describe() const {
        TextWriter stream;
        stream << name << " ( ";
        if ( subexpr && subexpr->type ) {
            stream << *subexpr->type;
        } else {
            stream << "???";
        }
        stream << ", ";
        if ( left && left->type ) {
            stream << *left->type;
        } else {
            stream << "???";
        }
        stream << ", ";
        if ( right && right->type ) {
            stream << *right->type;
        } else {
            stream << "???";
        }
        stream << " )";
        return stream.str();
    }


    // ExprMove

    ExpressionPtr ExprMove::visit(Visitor & vis) {
        vis.preVisit(this);
        left = left->visit(vis);
        vis.preVisitRight(this, right.get());
        right = right->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprMove::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprMove>(expr);
        ExprOp2::clone(cexpr);
        return cexpr;
    }

    
    // ExprClone

    ExpressionPtr ExprClone::visit(Visitor & vis) {
        vis.preVisit(this);
        left = left->visit(vis);
        vis.preVisitRight(this, right.get());
        right = right->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprClone::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprClone>(expr);
        ExprOp2::clone(cexpr);
        return cexpr;
    }

    // ExprCopy

    ExpressionPtr ExprCopy::visit(Visitor & vis) {
        vis.preVisit(this);
        left = left->visit(vis);
        vis.preVisitRight(this, right.get());
        right = right->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprCopy::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprCopy>(expr);
        ExprOp2::clone(cexpr);
        return cexpr;
    }

    // ExprTryCatch

    ExpressionPtr ExprTryCatch::visit(Visitor & vis) {
        vis.preVisit(this);
        try_block = try_block->visit(vis);
        vis.preVisitCatch(this,catch_block.get());
        catch_block = catch_block->visit(vis);
        return vis.visit(this);
    }

    uint32_t ExprTryCatch::getEvalFlags() const {
        return try_block->getEvalFlags() | catch_block->getEvalFlags();
    }

    ExpressionPtr ExprTryCatch::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprTryCatch>(expr);
        Expression::clone(cexpr);
        cexpr->try_block = try_block->clone();
        cexpr->catch_block = catch_block->clone();
        return cexpr;
    }

    // ExprReturn

    ExpressionPtr ExprReturn::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( subexpr ) {
            subexpr = subexpr->visit(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprReturn::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprReturn>(expr);
        Expression::clone(cexpr);
        if ( subexpr )
            cexpr->subexpr = subexpr->clone();
        cexpr->moveSemantics = moveSemantics;
        cexpr->fromYield = fromYield;
        return cexpr;
    }

    // ExprBreak

    ExpressionPtr ExprBreak::visit(Visitor & vis) {
        vis.preVisit(this);
        return vis.visit(this);
    }

    ExpressionPtr ExprBreak::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprBreak>(expr);
        Expression::clone(cexpr);
        return cexpr;
    }

    // ExprContinue

    ExpressionPtr ExprContinue::visit(Visitor & vis) {
        vis.preVisit(this);
        return vis.visit(this);
    }

    ExpressionPtr ExprContinue::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprContinue>(expr);
        Expression::clone(cexpr);
        return cexpr;
    }

    // ExprIfThenElse

    ExpressionPtr ExprIfThenElse::visit(Visitor & vis) {
        vis.preVisit(this);
        cond = cond->visit(vis);
        if ( vis.canVisitIfSubexpr(this) ) {
            vis.preVisitIfBlock(this, if_true.get());
            if_true = if_true->visit(vis);
            if ( if_false ) {
                vis.preVisitElseBlock(this, if_false.get());
                if_false = if_false->visit(vis);
            }
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprIfThenElse::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprIfThenElse>(expr);
        Expression::clone(cexpr);
        cexpr->cond = cond->clone();
        cexpr->if_true = if_true->clone();
        if ( if_false )
            cexpr->if_false = if_false->clone();
        cexpr->isStatic = isStatic;
        return cexpr;
    }

    uint32_t ExprIfThenElse::getEvalFlags() const {
        auto flagsE = cond->getEvalFlags() | if_true->getEvalFlags();
        if (if_false) flagsE |= if_false->getEvalFlags();
        return flagsE;
    }

    // ExprWith

    ExpressionPtr ExprWith::visit(Visitor & vis) {
        vis.preVisit(this);
        with = with->visit(vis);
        vis.preVisitWithBody(this, body.get());
        body = body->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprWith::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprWith>(expr);
        Expression::clone(cexpr);
        cexpr->with = with->clone();
        cexpr->body = body->clone();
        return cexpr;
    }

    // ExprWhile

    ExpressionPtr ExprWhile::visit(Visitor & vis) {
        vis.preVisit(this);
        cond = cond->visit(vis);
        vis.preVisitWhileBody(this, body.get());
        body = body->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprWhile::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprWhile>(expr);
        Expression::clone(cexpr);
        cexpr->cond = cond->clone();
        cexpr->body = body->clone();
        return cexpr;
    }

    uint32_t ExprWhile::getEvalFlags() const {
        return body->getEvalFlags() & ~(EvalFlags::stopForBreak | EvalFlags::stopForContinue);
    }

    // ExprFor

    ExpressionPtr ExprFor::visit(Visitor & vis) {
        vis.preVisit(this);
        for ( auto & var : iteratorVariables ) {
            vis.preVisitFor(this, var, var==iteratorVariables.back());
            var = vis.visitFor(this, var, var==iteratorVariables.back());
        }
        for ( auto & src : sources ) {
            vis.preVisitForSource(this, src.get(), src==sources.back());
            src = src->visit(vis);
            src = vis.visitForSource(this, src.get(), src==sources.back());
        }
        vis.preVisitForStack(this);
        if ( body ) {
            vis.preVisitForBody(this, body.get());
            body = body->visit(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprFor::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprFor>(expr);
        Expression::clone(cexpr);
        cexpr->iterators = iterators;
        for ( auto & src : sources )
            cexpr->sources.push_back(src->clone());
        for ( auto & var : iteratorVariables )
            cexpr->iteratorVariables.push_back(var->clone());
        if ( body ) {
            cexpr->body = body->clone();
        }
        return cexpr;
    }

    Variable * ExprFor::findIterator(const string & name) const {
        for ( auto & v : iteratorVariables ) {
            if ( v->name==name ) {
                return v.get();
            }
        }
        return nullptr;
    }

    uint32_t ExprFor::getEvalFlags() const {
        return body->getEvalFlags() & ~(EvalFlags::stopForBreak | EvalFlags::stopForContinue);
    }

    // ExprLet

    ExpressionPtr ExprLet::visit(Visitor & vis) {
        vis.preVisit(this);
        for ( auto it = variables.begin(); it!=variables.end(); ) {
            auto & var = *it;
            vis.preVisitLet(this, var, var==variables.back());
            if ( var->type ) {
                vis.preVisit(var->type.get());
                var->type = var->type->visit(vis);
                var->type = vis.visit(var->type.get());
            }
            if ( var->init ) {
                vis.preVisitLetInit(this, var, var->init.get());
                var->init = var->init->visit(vis);
                var->init = vis.visitLetInit(this, var, var->init.get());
            }
            var = vis.visitLet(this, var, var==variables.back());
            if ( var ) ++it; else it = variables.erase(it);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprLet::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprLet>(expr);
        Expression::clone(cexpr);
        for ( auto & var : variables )
            cexpr->variables.push_back(var->clone());
        cexpr->inScope = inScope;
        return cexpr;
    }

    Variable * ExprLet::find(const string & name) const {
        for ( auto & v : variables ) {
            if ( v->name==name ) {
                return v.get();
            }
        }
        return nullptr;
    }

    // ExprLooksLikeCall

    ExpressionPtr ExprLooksLikeCall::visit(Visitor & vis) {
        vis.preVisit(this);
        for ( auto & arg : arguments ) {
            vis.preVisitLooksLikeCallArg(this, arg.get(), arg==arguments.back());
            arg = arg->visit(vis);
            arg = vis.visitLooksLikeCallArg(this, arg.get(), arg==arguments.back());
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprLooksLikeCall::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprLooksLikeCall>(expr);
        Expression::clone(cexpr);
        cexpr->name = name;
        for ( auto & arg : arguments ) {
            cexpr->arguments.push_back(arg->clone());
        }
        return cexpr;
    }

    string ExprLooksLikeCall::describe() const {
        TextWriter stream;
        stream << name << " ( ";
        for ( auto & arg : arguments ) {
            if ( arg->type )
                stream << *arg->type;
            else
                stream << "???";
            if (arg != arguments.back()) {
                stream << ", ";
            }
        }
        stream << " )";
        return stream.str();
    }

    void ExprLooksLikeCall::autoDereference() {
        for ( size_t iA = 0; iA != arguments.size(); ++iA ) {
            arguments[iA] = Expression::autoDereference(arguments[iA]);
        }
    }

    // ExprCall

    ExpressionPtr ExprCall::visit(Visitor & vis) {
        vis.preVisit(this);
        for ( auto & arg : arguments ) {
            vis.preVisitCallArg(this, arg.get(), arg==arguments.back());
            arg = arg->visit(vis);
            arg = vis.visitCallArg(this, arg.get(), arg==arguments.back());
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprCall::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprCall>(expr);
        ExprLooksLikeCall::clone(cexpr);
        cexpr->func = func;
        return cexpr;
    }

    // named call

    ExpressionPtr ExprNamedCall::visit(Visitor & vis) {
        vis.preVisit(this);
        for ( auto & arg : arguments ) {
            vis.preVisitNamedCallArg(this, arg.get(), arg==arguments.back());
            arg->value = arg->value->visit(vis);
            arg = vis.visitNamedCallArg(this, arg.get(), arg==arguments.back());
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprNamedCall::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprNamedCall>(expr);
        Expression::clone(cexpr);
        cexpr->name = name;
        cexpr->arguments.reserve(arguments.size());
        for ( auto & arg : arguments ) {
            cexpr->arguments.push_back(arg->clone());
        }
        return cexpr;
    }

    // make structure

    MakeFieldDeclPtr MakeFieldDecl::clone() const {
        auto md = make_smart<MakeFieldDecl>();
        md->at = at;
        md->moveSemantic = moveSemantic;
        md->name = name;
        md->value = value->clone();
        return md;
    }

    ExpressionPtr ExprMakeStructureOrDefaultValue::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprMakeStructureOrDefaultValue>(expr);
        ExprMakeLocal::clone(cexpr);
        cexpr->structs.reserve ( structs.size() );
        for ( auto & fields : structs ) {
            auto mfd = make_smart<MakeStruct>();
            mfd->reserve( fields->size() );
            for ( auto & fd : *fields ) {
                mfd->push_back(fd->clone());
            }
            cexpr->structs.push_back(mfd);
        }
        cexpr->makeType = make_smart<TypeDecl>(*makeType);
        cexpr->useInitializer = useInitializer;
        return cexpr;
    }

    ExpressionPtr ExprMakeStructureOrDefaultValue::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( makeType ) {
            vis.preVisit(makeType.get());
            makeType = makeType->visit(vis);
            makeType = vis.visit(makeType.get());
        }
        for ( int index=0; index != int(structs.size()); ++index ) {
            vis.preVisitMakeStructureIndex(this, index, index==int(structs.size()-1));
            auto & fields = structs[index];
            for ( auto it = fields->begin(); it != fields->end(); ) {
                auto & field = *it;
                vis.preVisitMakeStructureField(this, index, field.get(), field==fields->back());
                field->value = field->value->visit(vis);
                if ( field ) {
                    field = vis.visitMakeStructureField(this, index, field.get(), field==fields->back());
                }
                if ( field ) ++it; else it = fields->erase(it);
            }
            vis.visitMakeStructureIndex(this, index, index==int(structs.size()-1));
        }
        return vis.visit(this);
    }

    // make variant

    ExpressionPtr ExprMakeVariant::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprMakeVariant>(expr);
        ExprMakeLocal::clone(cexpr);
        cexpr->variants.reserve ( variants.size() );
        for ( auto & fd : variants ) {
            cexpr->variants.push_back(fd->clone());
        }
        cexpr->makeType = make_smart<TypeDecl>(*makeType);
        return cexpr;
    }

    ExpressionPtr ExprMakeVariant::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( makeType ) {
            vis.preVisit(makeType.get());
            makeType = makeType->visit(vis);
            makeType = vis.visit(makeType.get());
        }
        int index = 0;
        for ( auto it = variants.begin(); it != variants.end(); index ++ ) {
            auto & field = *it;
            vis.preVisitMakeVariantField(this, index, field.get(), field==variants.back());
            field->value = field->value->visit(vis);
            if ( field ) {
                field = vis.visitMakeVariantField(this, index, field.get(), field==variants.back());
            }
            if ( field ) ++it; else it = variants.erase(it);
        }
        return vis.visit(this);
    }

    // make array

    ExpressionPtr ExprMakeArray::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprMakeArray>(expr);
        ExprMakeLocal::clone(cexpr);
        cexpr->values.reserve ( values.size() );
        for ( auto & val : values ) {
            cexpr->values.push_back(val->clone());
        }
        cexpr->makeType = make_smart<TypeDecl>(*makeType);
        return cexpr;
    }

    ExpressionPtr ExprMakeArray::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( makeType ) {
            vis.preVisit(makeType.get());
            makeType = makeType->visit(vis);
            makeType = vis.visit(makeType.get());
        }
        int index = 0;
        for ( auto it = values.begin(); it != values.end(); ) {
            auto & value = *it;
            vis.preVisitMakeArrayIndex(this, index, value.get(), index==int(values.size()-1));
            value = value->visit(vis);
            if ( value ) {
                value = vis.visitMakeArrayIndex(this, index, value.get(), index==int(values.size()-1));
            }
            if ( value ) ++it; else it = values.erase(it);
            index ++;
        }
        return vis.visit(this);
    }

    // make tuple

    ExpressionPtr ExprMakeTuple::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprMakeTuple>(expr);
        ExprMakeLocal::clone(cexpr);
        cexpr->values.reserve ( values.size() );
        for ( auto & val : values ) {
            cexpr->values.push_back(val->clone());
        }
        if ( makeType ) {
            cexpr->makeType = make_smart<TypeDecl>(*makeType);
        }
        cexpr->isKeyValue = isKeyValue;
        return cexpr;
    }

    ExpressionPtr ExprMakeTuple::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( makeType ) {
            vis.preVisit(makeType.get());
            makeType = makeType->visit(vis);
            makeType = vis.visit(makeType.get());
        }
        int index = 0;
        for ( auto it = values.begin(); it != values.end(); ) {
            auto & value = *it;
            vis.preVisitMakeTupleIndex(this, index, value.get(), index==int(values.size()-1));
            value = value->visit(vis);
            if ( value ) {
                value = vis.visitMakeTupleIndex(this, index, value.get(), index==int(values.size()-1));
            }
            if ( value ) ++it; else it = values.erase(it);
            index ++;
        }
        return vis.visit(this);
    }


    // array comprehension

    ExpressionPtr ExprArrayComprehension::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprArrayComprehension>(expr);
        Expression::clone(cexpr);
        cexpr->exprFor = exprFor->clone();
        cexpr->subexpr = subexpr->clone();
        cexpr->generatorSyntax = generatorSyntax;
        if ( exprWhere ) {
            cexpr->exprWhere = exprWhere->clone();
        }
        return cexpr;
    }

    ExpressionPtr ExprArrayComprehension::visit(Visitor & vis) {
        vis.preVisit(this);
        exprFor = exprFor->visit(vis);
        vis.preVisitArrayComprehensionSubexpr(this, subexpr.get());
        subexpr = subexpr->visit(vis);
        if ( exprWhere ) {
            vis.preVisitArrayComprehensionWhere(this, exprWhere.get());
            exprWhere = exprWhere->visit(vis);
        }
        return vis.visit(this);
    }

    // program

    int Program::getContextStackSize() const {
        return options.getIntOption("stack", policies.stack);
    }

    vector<EnumerationPtr> Program::findEnum ( const string & name ) const {
        return library.findEnum(name,thisModule.get());
    }

    vector<AnnotationPtr> Program::findAnnotation ( const string & name ) const {
        return library.findAnnotation(name,thisModule.get());
    }

    vector<StructurePtr> Program::findStructure ( const string & name ) const {
        return library.findStructure(name,thisModule.get());
    }

    void Program::error ( const string & str, const string & extra, const string & fixme, const LineInfo & at, CompilationError cerr ) {
        errors.emplace_back(str,extra,fixme,at,cerr);
        failToCompile = true;
    }

    Module * Program::addModule ( const string & name ) {
        if ( auto lm = library.findModule(name) ) {
            return lm;
        } else {
            if ( auto pm = Module::require(name) ) {
                library.addModule(pm);
                return pm;
            } else {
                return nullptr;
            }
        }
    }

    bool Program::addAlias ( const TypeDeclPtr & at ) {
        return thisModule->addAlias(at, true);
    }

    bool Program::addVariable ( const VariablePtr & var ) {
        return thisModule->addVariable(var, true);
    }

    bool Program::addStructure ( const StructurePtr & st ) {
        return thisModule->addStructure(st, true);
    }

    bool Program::addEnumeration ( const EnumerationPtr & st ) {
        return thisModule->addEnumeration(st, true);
    }

    FunctionPtr Program::findFunction(const string & mangledName) const {
        return thisModule->findFunction(mangledName);
    }

    bool Program::addFunction ( const FunctionPtr & fn ) {
        return thisModule->addFunction(fn, true);
    }

    bool Program::addGeneric ( const FunctionPtr & fn ) {
        return thisModule->addGeneric(fn, true);
    }

    bool Program::addStructureHandle ( const StructurePtr & st, const TypeAnnotationPtr & ann, const AnnotationArgumentList & arg ) {
        if ( ann->rtti_isStructureTypeAnnotation() ) {
            auto annotation = static_pointer_cast<StructureTypeAnnotation>(ann->clone());
            annotation->name = st->name;
            string err;
            if ( annotation->create(st,arg,err) ) {
                return thisModule->addAnnotation(annotation, true);
            } else {
                error("can't create structure handle "+ann->name,err,"",st->at,CompilationError::invalid_annotation);
                return false;
            }
        } else {
            error("not a structure annotation "+ann->name,"","",st->at,CompilationError::invalid_annotation);
            return false;
        }
    }

    Program::Program() {
        thisModule = make_unique<ModuleDas>();
        library.addBuiltInModule();
        library.addModule(thisModule.get());
    }

    TypeDecl * Program::makeTypeDeclaration(const LineInfo &at, const string &name) {
        auto structs = library.findStructure(name,thisModule.get());
        auto handles = library.findAnnotation(name,thisModule.get());
        auto enums = library.findEnum(name,thisModule.get());
        auto aliases = library.findAlias(name,thisModule.get());
        if ( ((structs.size()!=0)+(handles.size()!=0)+(enums.size()!=0)+(aliases.size()!=0)) > 1 ) {
            string candidates = describeCandidates(structs);
            candidates += describeCandidates(handles, false);
            candidates += describeCandidates(enums, false);
            candidates += describeCandidates(aliases, false);
            error("undefined type "+name,candidates,"",
                at,CompilationError::type_not_found);
            return nullptr;
        } else if ( structs.size() ) {
            if ( structs.size()==1 ) {
                auto pTD = new TypeDecl(structs.back());
                pTD->at = at;
                return pTD;
            } else {
                string candidates = describeCandidates(structs);
                error("too many options for "+name,candidates,"",
                    at,CompilationError::structure_not_found);
                return nullptr;
            }
        } else if ( handles.size() ) {
            if ( handles.size()==1 ) {
                if ( handles.back()->rtti_isHandledTypeAnnotation() ) {
                    auto pTD = new TypeDecl(Type::tHandle);
                    pTD->annotation = static_cast<TypeAnnotation *>(handles.back().get());
                    pTD->at = at;
                    return pTD;
                } else {
                    error("not a handled type annotation "+name,"","",
                        at,CompilationError::handle_not_found);
                    return nullptr;
                }
            } else {
                string candidates = describeCandidates(handles);
                error("too many options for "+name, candidates, "",
                    at,CompilationError::handle_not_found);
                return nullptr;
            }
        } else if ( enums.size() ) {
            if ( enums.size()==1 ) {
                auto pTD = new TypeDecl(enums.back());
                pTD->enumType = enums.back().get();
                pTD->at = at;
                return pTD;
            } else {
                string candidates = describeCandidates(enums);
                error("too many options for "+name,candidates,"",
                    at,CompilationError::enumeration_not_found);
                return nullptr;
            }
        } else if ( aliases.size() ) {
            if ( aliases.size()==1 ) {
                auto pTD = new TypeDecl(*aliases.back());
                pTD->at = at;
                return pTD;
            } else {
                string candidates = describeCandidates(aliases);
                error("too many options for "+name,candidates,"",
                    at,CompilationError::type_alias_not_found);
                return nullptr;
            }
        } else {
            auto tt = new TypeDecl(Type::alias);
            tt->alias = name;
            tt->at = at;
            return tt;
        }
    }

    ExprLooksLikeCall * Program::makeCall ( const LineInfo & at, const string & name ) {
        vector<ExprCallFactory *> ptr;
        string moduleName, funcName;
        splitTypeName(name, moduleName, funcName);
        library.foreach([&](Module * pm) -> bool {
            if ( auto pp = pm->findCall(funcName) )
                ptr.push_back(pp);
            return false;
        }, moduleName);
        if ( ptr.size()==1 ) {
            return (*ptr.back())(at);
        } else if ( ptr.size()==0 ) {
            return new ExprCall(at,name);
        } else {
            error("too many options for " + name,"","", at, CompilationError::function_not_found);
            return new ExprCall(at,name);
        }
    }

    int64_t getConstExprIntOrUInt ( const ExpressionPtr & expr ) {
        DAS_ASSERTF ( expr && expr->rtti_isConstant(),
                     "expecting constant. something in enumeration (or otherwise) did not fold.");
        auto econst = static_pointer_cast<ExprConst>(expr);
        switch (econst->baseType) {
        case Type::tInt8:   return static_pointer_cast<ExprConstInt8>(expr)->getValue();
        case Type::tUInt8:  return static_pointer_cast<ExprConstUInt8>(expr)->getValue();
        case Type::tInt16:  return static_pointer_cast<ExprConstInt16>(expr)->getValue();
        case Type::tUInt16: return static_pointer_cast<ExprConstUInt16>(expr)->getValue();
        case Type::tInt:    return static_pointer_cast<ExprConstInt>(expr)->getValue();
        case Type::tUInt:   return static_pointer_cast<ExprConstUInt>(expr)->getValue();
        case Type::tInt64:  return static_pointer_cast<ExprConstInt64>(expr)->getValue();
        case Type::tUInt64: return static_pointer_cast<ExprConstUInt64>(expr)->getValue();
        default:
            DAS_ASSERTF ( 0,
                "we should not even be here. there is an enumeration of unsupported type."
                "something in enumeration (or otherwise) did not fold.");
            return 0;
        }
    }

    ExpressionPtr Program::makeConst ( const LineInfo & at, const TypeDeclPtr & type, vec4f value ) {
        if ( type->dim.size() || type->ref ) return nullptr;
        switch ( type->baseType ) {
            case Type::tBool:           return make_smart<ExprConstBool>(at, cast<bool>::to(value));
            case Type::tInt8:           return make_smart<ExprConstInt8>(at, cast<int8_t>::to(value));
            case Type::tInt16:          return make_smart<ExprConstInt16>(at, cast<int16_t>::to(value));
            case Type::tInt64:          return make_smart<ExprConstInt64>(at, cast<int64_t>::to(value));
            case Type::tInt:            return make_smart<ExprConstInt>(at, cast<int32_t>::to(value));
            case Type::tInt2:           return make_smart<ExprConstInt2>(at, cast<int2>::to(value));
            case Type::tInt3:           return make_smart<ExprConstInt3>(at, cast<int3>::to(value));
            case Type::tInt4:           return make_smart<ExprConstInt4>(at, cast<int4>::to(value));
            case Type::tUInt8:          return make_smart<ExprConstUInt8>(at, cast<uint8_t>::to(value));
            case Type::tUInt16:         return make_smart<ExprConstUInt16>(at, cast<uint16_t>::to(value));
            case Type::tUInt64:         return make_smart<ExprConstUInt64>(at, cast<uint64_t>::to(value));
            case Type::tUInt:           return make_smart<ExprConstUInt>(at, cast<uint32_t>::to(value));
            case Type::tUInt2:          return make_smart<ExprConstUInt2>(at, cast<uint2>::to(value));
            case Type::tUInt3:          return make_smart<ExprConstUInt3>(at, cast<uint3>::to(value));
            case Type::tUInt4:          return make_smart<ExprConstUInt4>(at, cast<uint4>::to(value));
            case Type::tFloat:          return make_smart<ExprConstFloat>(at, cast<float>::to(value));
            case Type::tFloat2:         return make_smart<ExprConstFloat2>(at, cast<float2>::to(value));
            case Type::tFloat3:         return make_smart<ExprConstFloat3>(at, cast<float3>::to(value));
            case Type::tFloat4:         return make_smart<ExprConstFloat4>(at, cast<float4>::to(value));
            case Type::tDouble:         return make_smart<ExprConstDouble>(at, cast<double>::to(value));
            case Type::tRange:          return make_smart<ExprConstRange>(at, cast<range>::to(value));
            case Type::tURange:         return make_smart<ExprConstURange>(at, cast<urange>::to(value));
            default:                    DAS_ASSERTF(0, "we should not even be here"); return nullptr;
        }
    }

    StructurePtr Program::visitStructure(Visitor & vis, Structure * pst) {
        vis.preVisit(pst);
        for ( auto & fi : pst->fields ) {
            vis.preVisitStructureField(pst, fi, &fi==&pst->fields.back());
            if ( fi.type ) {
                vis.preVisit(fi.type.get());
                fi.type = fi.type->visit(vis);
                fi.type = vis.visit(fi.type.get());
            }
            if ( fi.init && vis.canVisitStructureFieldInit(pst) ) {
                fi.init = fi.init->visit(vis);
            }
            vis.visitStructureField(pst, fi, &fi==&pst->fields.back());
        }
        return vis.visit(pst);
    }

    EnumerationPtr Program::visitEnumeration(Visitor & vis, Enumeration * penum) {
        vis.preVisit(penum);
        size_t count = 0;
        size_t total = penum->list.size();
        for ( auto & itf : penum->list ) {
            vis.preVisitEnumerationValue(penum, itf.first, itf.second.get(), count==total);
            if ( itf.second ) itf.second = itf.second->visit(vis);
            itf.second = vis.visitEnumerationValue(penum, itf.first, itf.second.get(), count==total);
            count ++;
        }
        return vis.visit(penum);
    }

    void Program::visit(Visitor & vis, bool visitGenerics ) {
        // before program
        vis.preVisitProgram(this);
        // enumerations
        for ( auto & ite : thisModule->enumerations ) {
            ite.second = visitEnumeration(vis, ite.second.get());
        }
        // structures
        for ( auto & ist : thisModule->structuresInOrder ) {
            Structure * pst = ist.get();
            StructurePtr pstn = visitStructure(vis, pst);
            if ( pstn.get() != pst ) {
                assert(pstn->name==pst->name);
                auto istm = thisModule->structures.find(pst->name);
                assert ( istm!=thisModule->structures.end() );
                istm->second = pstn;
                ist = pstn;
            }
        }
        // aliases
        for ( auto & als : thisModule->aliasTypes ) {
            vis.preVisit(als.second.get());
            als.second = als.second->visit(vis);
            als.second = vis.visit(als.second.get());
        }
        // real things
        vis.preVisitProgramBody(this);
        // globals
        vis.preVisitGlobalLetBody(this);
        for ( auto & var : thisModule->globalsInOrder ) {
            vis.preVisitGlobalLet(var);
            if ( var->type ) {
                vis.preVisit(var->type.get());
                var->type = var->type->visit(vis);
                var->type = vis.visit(var->type.get());
            }
            if ( var->init ) {
                vis.preVisitGlobalLetInit(var, var->init.get());
                var->init = var->init->visit(vis);
                var->init = vis.visitGlobalLetInit(var, var->init.get());
            }
            var = vis.visitGlobalLet(var);
        }
        vis.visitGlobalLetBody(this);
        // generics
        if ( visitGenerics ) {
            for ( auto & fn : thisModule->generics ) {
                if ( !fn.second->builtIn ) {
                    fn.second = fn.second->visit(vis);
                }
            }
        }
        // functions
        for ( auto & fn : thisModule->functions ) {
            if ( !fn.second->builtIn ) {
                if ( vis.canVisitFunction(fn.second.get()) ) {
                    auto res = fn.second->visit(vis);
                    fn.second = res;
                }
            }
        }
        // done
        vis.visitProgram(this);
    }

    void Program::optimize(TextWriter & logs, ModuleGroup & libGroup) {
        const bool log = options.getBoolOption("log_optimization_passes",false);
        bool any, last;
        if (log) {
            logs << *this << "\n";
        }
        do {
            if ( log ) logs << "OPTIMIZE:\n" << *this;
            any = false;
            last = optimizationRefFolding();    if ( failed() ) break;  any |= last;
            if ( log ) logs << "REF FOLDING: " << (last ? "optimized" : "nothing") << "\n" << *this;
            last = optimizationUnused(logs);    if ( failed() ) break;  any |= last;
            if ( log ) logs << "REMOVE UNUSED:" << (last ? "optimized" : "nothing") << "\n" << *this;
            last = optimizationConstFolding();  if ( failed() ) break;  any |= last;
            if ( log ) logs << "CONST FOLDING:" << (last ? "optimized" : "nothing") << "\n" << *this;
            last = optimizationCondFolding();  if ( failed() ) break;  any |= last;
            if ( log ) logs << "COND FOLDING:" << (last ? "optimized" : "nothing") << "\n" << *this;
            last = optimizationBlockFolding();  if ( failed() ) break;  any |= last;
            if ( log ) logs << "BLOCK FOLDING:" << (last ? "optimized" : "nothing") << "\n" << *this;
            // this is here again for a reason
            last = optimizationUnused(logs);    if ( failed() ) break;  any |= last;
            if ( log ) logs << "REMOVE UNUSED:" << (last ? "optimized" : "nothing") << "\n" << *this;
            // now, user macros
            last = false;
            auto modMacro = [&](Module * mod) -> bool {    // we run all macros for each module
                for ( const auto & pm : mod->optimizationMacros ) {
                    this->visit(*pm);
                    if ( failed() ) {                       // if macro failed, we report it, and we are done
                        error("optimization macro " + mod->name + "::" + pm->macroName() + " failed", "","",LineInfo());
                        return false;
                    }
                    last |= pm->didAnything();
                }
                return true;
            };
            Module::foreach(modMacro);
            if ( failed() ) break;  any |= last;
            libGroup.foreach(modMacro,"*");
            if ( failed() ) break;  any |= last;
            if ( log ) logs << "MACROS:" << (last ? "optimized" : "nothing") << "\n" << *this;
        } while ( any );
    }
}
