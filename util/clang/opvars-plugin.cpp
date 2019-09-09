// Use with:
// -Xclang -load -Xclang path/to/opvars-plugin.so -Xclang -add-plugin -Xclang opvars-plugin

#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <iostream>

#define __STDC_LIMIT_MACROS 1
#define __STDC_CONSTANT_MACROS 1
#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/Version.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendPluginRegistry.h>

#include "compat.h"

namespace {

class OpVarsVisitor : public clang::RecursiveASTVisitor<OpVarsVisitor> {
public:
    explicit OpVarsVisitor(clang::CompilerInstance& inst) :
    diagnostics(inst.getDiagnostics())
    {
        arg_wrong_type = diagnostics.getCustomDiagID(
                clang::DiagnosticsEngine::Error,
                "argument %0 to '%1' has type '%2', but format character '%3' expects '%4'");
        arg_not_constant = diagnostics.getCustomDiagID(
                clang::DiagnosticsEngine::Error,
                "argument %0 to '%1' is not a string literal");
        arg_bad_character = diagnostics.getCustomDiagID(
                clang::DiagnosticsEngine::Error,
                "Invalid format character '%0'");
    }

    bool VisitCallExpr(clang::CallExpr *exp)
    {
        clang::FunctionDecl *d = exp->getDirectCallee();
        if (d == nullptr) { return true; }

        // Determine whether the function is one we're looking for
        auto ident = d->getIdentifier();
        if (ident == nullptr) { return true; }
        llvm::StringRef const name = ident->getName();
        if (name == "add_opvars") {
            // Argument 2 must be a string literal
            clang::Expr const *arg = exp->getArg(1);
            arg = arg->IgnoreImplicit();
            clang::StringLiteral const *str_p = llvm::dyn_cast_or_null<clang::StringLiteral>(arg);
            if (str_p == nullptr
                || (str_p->getKind() != clang::StringLiteral::Ascii
                 && str_p->getKind() != clang::StringLiteral::UTF8)) {
                std::cout << "str_p is " << (void *)str_p;
                if (str_p)
                    std::cout << " getKind() is " << str_p->getKind();
                std::cout << "\n";
                diagnostics.Report(arg->getLocStart(), arg_not_constant) << 2 << name;
            } else {
                std::string str = str_p->getString().str();
                std::size_t argnum = 2;
                const char *target1 = "", *target2 = "";

                for (std::size_t i = 0; i < str.size(); ++i) {
                    char ch = str.at(i);
                    switch (ch) {
                    case ' ':
                        continue;

                    case 'i': /* integer */
                    case 'o': /* opcode */
                        target1 = "int";
                        target2 = "unsigned";
                        break;

                    case 'l': /* integer */
                    case 'c': /* coordinate */
                    case 'r': /* region */
                    case 'm': /* mapchar */
                    case 'M': /* monster */
                    case 'O': /* object */
                        target1 = "long";
                        target2 = "unsigned long";
                        break;

                    case 's': /* string */
                    case 'v': /* variable */
                        target1 = "const char *";
                        target2 = "char *";
                        break;

                    default:
                        diagnostics.Report(arg->getLocStart(), arg_bad_character) << str.substr(i, 1);
                        continue;
                    }
                    clang::Expr const *arg2 = exp->getArg(argnum++);
                    auto tname = arg2->getType().getAsString();
                    if (tname != target1 && tname != target2) {
                        diagnostics.Report(arg2->getLocStart(), arg_wrong_type)
                        << static_cast<unsigned>(argnum)
                        << name << tname << str.substr(i, 1) << target1;
                    }
                }
            }
        }

        return true;
    }

private:
    clang::DiagnosticsEngine& diagnostics;
    unsigned arg_wrong_type;
    unsigned arg_not_constant;
    unsigned arg_bad_character;
};

struct OpVarsASTConsumer : public clang::ASTConsumer {
public:
    explicit OpVarsASTConsumer(clang::CompilerInstance& inst)
    : Visitor(inst) {}

    virtual void HandleTranslationUnit(clang::ASTContext &Context)
    {
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }
private:
    OpVarsVisitor Visitor;
};

class OpVarsAction : public clang::PluginASTAction {
public:
	clang_compat::ASTConsumerPtr CreateASTConsumer(clang::CompilerInstance& inst, llvm::StringRef str) override
    {
        return clang_compat::ASTConsumerPtr(new OpVarsASTConsumer(inst));
    }

    bool ParseArgs(const clang::CompilerInstance& inst,
                   const std::vector<std::string>& args) override
    {
        return true;
    }
};

}

static clang::FrontendPluginRegistry::Add<OpVarsAction>
X("opvars-plugin", "check add_opvars arguments");
