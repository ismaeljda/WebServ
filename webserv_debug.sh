#!/bin/bash

# Script de debug spécifique pour les erreurs Webserv
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_issue() {
    echo -e "${RED}❌ PROBLÈME DÉTECTÉ: $1${NC}"
    echo -e "${YELLOW}   Solution: $2${NC}"
    echo ""
}

print_check() {
    echo -e "${BLUE}🔍 Vérification: $1${NC}"
}

print_ok() {
    echo -e "${GREEN}✅ $1${NC}"
}

echo -e "${BLUE}╔══════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║     ANALYSE DES ERREURS WEBSERV             ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════╝${NC}"
echo ""

# 1. Vérifier la fonction trim dupliquée
print_check "Fonction trim() dupliquée"
trim_count=$(grep -r "std::string.*trim.*(" . --include="*.cpp" --include="*.hpp" 2>/dev/null | wc -l)
if [ $trim_count -gt 1 ]; then
    print_issue "Fonction trim() définie $trim_count fois" "Créer un fichier Utils.hpp/Utils.cpp commun"
    echo "Fichiers concernés:"
    grep -r "std::string.*trim.*(" . --include="*.cpp" --include="*.hpp" 2>/dev/null
    echo ""
else
    print_ok "Pas de duplication de trim() détectée"
fi

# 2. Vérifier les problèmes CGI
print_check "Variables d'environnement CGI"
if grep -q 'env.push_back("QUERY_STRING" +' Server.cpp 2>/dev/null; then
    print_issue "Variable QUERY_STRING mal formatée dans handle_cgi()" "Changer en 'env.push_back(\"QUERY_STRING=\" + req.getQueryParamsAsString());'"
    grep -n 'QUERY_STRING.*+' Server.cpp
    echo ""
else
    print_ok "Variable QUERY_STRING correctement formatée"
fi

# 3. Vérifier les problèmes de Content-Type CGI
print_check "Gestion du Content-Type dans CGI"
if grep -A5 -B5 "req.getHeaders().find(\"Content-Type\")" Server.cpp | grep -q "->second" 2>/dev/null; then
    print_issue "Accès direct à Content-Type sans vérification" "Vérifier si l'iterator != end() avant d'accéder à ->second"
    grep -n "Content-Type.*->second" Server.cpp
    echo ""
else
    print_ok "Accès sécurisé au header Content-Type"
fi

# 4. Vérifier les exit() dans le constructeur
print_check "Appels exit() dans le constructeur"
if grep -q "exit(EXIT_FAILURE)" Server.cpp 2>/dev/null; then
    print_issue "Appels exit() dans le constructeur Server" "Remplacer par throw std::runtime_error() pour une gestion d'erreur propre"
    grep -n "exit(EXIT_FAILURE)" Server.cpp
    echo ""
else
    print_ok "Pas d'appels exit() problématiques"
fi

# 5. Vérifier la validation des méthodes HTTP
print_check "Validation des méthodes HTTP par location"
if grep -A20 "handle_request" Server.cpp | grep -q "find.*allow_methods.*end" 2>/dev/null; then
    line_num=$(grep -n "find.*allow_methods" Server.cpp | cut -d: -f1)
    method_check_line=$(grep -n "if.*getMethod.*GET\|POST\|DELETE" Server.cpp | cut -d: -f1)
    if [ "$method_check_line" -lt "$line_num" ]; then
        print_issue "Validation des méthodes après traitement" "Déplacer la validation des allow_methods avant le traitement de la requête"
        echo "Ligne de traitement: $method_check_line, Ligne de validation: $line_num"
        echo ""
    else
        print_ok "Validation des méthodes correctement placée"
    fi
else
    print_ok "Validation des méthodes semble correcte"
fi

# 6. Vérifier les fuites de file descriptors
print_check "Gestion des file descriptors dans CGI"
cgi_function=$(sed -n '/void Server::handle_cgi/,/^}/p' Server.cpp 2>/dev/null)
if echo "$cgi_function" | grep -q "pipe.*in_pipe\|out_pipe" && ! echo "$cgi_function" | grep -q "close.*in_pipe\[0\].*close.*in_pipe\[1\]"; then
    print_issue "Possible fuite de file descriptors dans handle_cgi" "S'assurer que tous les pipes sont fermés dans tous les cas d'erreur"
    echo ""
else
    print_ok "Gestion des file descriptors semble correcte"
fi

# 7. Vérifier la gestion des processus zombies
print_check "Gestion des processus zombies CGI"
if grep -q "waitpid.*pid.*NULL.*0" Server.cpp 2>/dev/null; then
    print_ok "waitpid() utilisé pour éviter les processus zombies"
else
    print_issue "Pas de waitpid() trouvé" "Ajouter waitpid(pid, NULL, 0) après l'exécution CGI"
fi

# 8. Vérifier les headers HTTP manquants
print_check "Headers HTTP obligatoires"
if ! grep -q "Server:" Server.cpp 2>/dev/null; then
    print_issue "Header Server manquant" "Ajouter 'header << \"Server: webserv/1.0\\r\\n\";' dans les réponses"
fi

if ! grep -q "Connection:" Server.cpp 2>/dev/null; then
    print_issue "Header Connection manquant" "Ajouter 'header << \"Connection: close\\r\\n\";' dans toutes les réponses"
fi

# 9. Vérifier la gestion des timeouts
print_check "Gestion des timeouts"
if ! grep -q "timeout\|alarm\|select.*timeout" Server.cpp main.cpp 2>/dev/null; then
    print_issue "Pas de timeout détecté" "Implémenter un timeout pour les requêtes et les CGI"
    echo ""
else
    print_ok "Timeout semble implémenté"
fi

# 10. Vérifier la validation des headers
print_check "Validation de la taille des headers"
if ! grep -q "header.*size.*>\|length.*>" RequestParser.cpp Server.cpp 2>/dev/null; then
    print_issue "Pas de limite sur la taille des headers" "Ajouter une vérification de la taille maximale des headers"
    echo ""
else
    print_ok "Validation de taille semble présente"
fi

# 11. Vérifier les problèmes de buffer overflow
print_check "Protection contre buffer overflow"
if grep -q "char buffer\[" Server.cpp 2>/dev/null; then
    print_issue "Utilisation de buffer fixe" "Vérifier que requestStr.append() ne cause pas de débordement"
    grep -n "char buffer\[" Server.cpp
    echo ""
else
    print_ok "Pas de buffer fixe détecté"
fi

# 12. Vérifier le script Python CGI
print_check "Script Python CGI"
if [ -f "*.py" ] && grep -q "__name__.*==.*__main__" *.py 2>/dev/null; then
    if grep -q "\*\*name\*\*" *.py 2>/dev/null; then
        print_issue "Erreur de syntaxe dans le script Python" "Remplacer **name** par __name__"
        grep -n "\*\*name\*\*" *.py
        echo ""
    else
        print_ok "Script Python syntaxiquement correct"
    fi
fi

# 13. Test de compilation spécifique
print_check "Test de compilation avec warnings"
echo "Compilation en cours..."
if make re > /tmp/webserv_compile.log 2>&1; then
    if grep -i "warning\|error" /tmp/webserv_compile.log; then
        print_issue "Warnings/erreurs de compilation détectés" "Corriger tous les warnings avant soumission"
        echo ""
    else
        print_ok "Compilation sans warnings"
    fi
else
    print_issue "Échec de compilation" "Voir les erreurs ci-dessous"
    cat /tmp/webserv_compile.log
fi

echo -e "\n${BLUE}╔══════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║              RÉSUMÉ DES CHECKS               ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${YELLOW}Si des problèmes ont été détectés, corrigez-les avant l'évaluation.${NC}"
echo -e "${YELLOW}Utilisez le script de test principal pour valider les corrections.${NC}"

# Nettoyage
rm -f /tmp/webserv_compile.log