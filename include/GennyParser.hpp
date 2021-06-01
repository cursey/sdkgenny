#pragma once

#include <tao/pegtl.hpp>

#include "Genny.hpp"

namespace genny::parser {
    using namespace tao::pegtl;

    struct Comment : disable<one<'#'>, until<eolf>> {};
    struct Ws : star<sor<space, Comment>> {};

    struct HexNum : seq<one<'0'>, one<'x'>, plus<xdigit>> {};
    struct DecNum: plus<digit> {};
    struct Num : sor<HexNum, DecNum> {};

    struct TypeId: TAO_PEGTL_STRING("type") {};
    struct TypeName : identifier {};
    struct TypeSize : Num {};
    struct TypeDecl : seq<TypeId, Ws, TypeName, Ws, TypeSize> {};

    struct StructId : TAO_PEGTL_STRING("struct") {};
    struct StructName : identifier {};
    struct StructParent : identifier {};
    struct StructParentList : seq<StructParent, opt_must<one<','>, Ws, StructParent>> {};
    struct StructParentListDecl : seq<one<':'>, Ws, StructParentList> {}; 
    struct StructDecl : seq<StructId, Ws, StructName, Ws, opt<StructParentListDecl>> {};

    struct VarType : identifier {};
    struct VarName : identifier {};
    struct VarOffset: Num {};
    struct VarOffsetDecl : seq<one<'@'>, Ws, VarOffset> {};
    struct VarDecl : seq<VarType, Ws, VarName, Ws, opt<VarOffsetDecl>> {};

    struct Decl : seq<Ws, sor<TypeDecl, StructDecl, VarDecl>> {};

    struct Grammar : until<eof, sor<eolf, Decl>> {};
}
