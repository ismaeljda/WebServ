#!/bin/bash

# Script de validation pour soumission Webserv (École 42)
# Ce script teste UNIQUEMENT les exigences obligatoires du sujet

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

WEBSERV_PID=""
TEST_PORT=8080
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
CRITICAL_ERRORS=0

print_header() {
    echo -e "${BLUE}════════════════════════════════════════${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}════════════════════════════════════════${NC}"
}

print_test() {
    echo -e "${YELLOW}[TEST] $1${NC}"
    ((TOTAL_TESTS++))
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
    ((PASSED_TESTS++))
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
    ((FAILED_TESTS++))
}

print_critical() {
    echo -e "${RED}🚨 CRITIQUE: $1${NC}"
    ((FAILED_TESTS++))
    ((CRITICAL_ERRORS++))
}

cleanup() {
    if [ ! -z "$WEBSERV_PID" ]; then
        kill -9 $WEBSERV_PID 2>/dev/null
        wait $WEBSERV_PID 2>/dev/null
    fi
    rm -rf ./test_validation
}

trap cleanup EXIT

print_header "VALIDATION WEBSERV - REQUIREMENTS 42"
echo -e "${BLUE}Tests basés sur les exigences strictes du sujet${NC}\n"

# ============================================================================
# 1. TESTS DE COMPILATION OBLIGATOIRES
# ============================================================================
print_header "1. COMPILATION (OBLIGATOIRE)"

print_test "Makefile existe"
if [ -f "Makefile" ]; then
    print_success "Makefile présent"
else
    print_critical "Makefile manquant"
fi

print_test "Règles Makefile obligatoires"
required_rules=("all" "clean" "fclean" "re")
for rule in "${required_rules[@]}"; do
    if grep -q "^$rule:" Makefile; then
        print_success "Règle $rule présente"
    else
        print_critical "Règle $rule manquante dans Makefile"
    fi
done

print_test "Compilation avec flags obligatoires"
if make re > /dev/null 2>&1; then
    print_success "Compilation réussie"
else
    print_critical "Échec de compilation - PROJET NON VALIDE"
    echo "Erreurs de compilation:"
    make re
    exit 1
fi

print_test "Aucun warning de compilation"
compile_output=$(make 2>&1)
if echo "$compile_output" | grep -qi "warning"; then
    print_error "Warnings détectés (peut causer un échec)"
    echo "$compile_output" | grep -i "warning"
else
    print_success "Aucun warning"
fi

print_test "Binaire webserv créé"
if [ -f "./webserv" ] || [ -f "./WebServ" ]; then
    BINARY_NAME="webserv"
    [ -f "./WebServ" ] && BINARY_NAME="WebServ"
    print_success "Binaire $BINARY_NAME créé"
else
    print_critical "Aucun binaire webserv trouvé"
fi

# ============================================================================
# 2. TESTS DE FICHIER DE CONFIGURATION
# ============================================================================
print_header "2. FICHIER DE CONFIGURATION (OBLIGATOIRE)"

# Créer un environnement de test minimal
mkdir -p test_validation/www
echo "<html><body><h1>Test Page</h1></body></html>" > test_validation/www/index.html
echo "<html><body><h1>Test Page 2</h1></body></html>" > test_validation/www/test.html

cat > test_validation/test.conf << 'EOF'
server {
    listen 8080;
    server_name localhost;
    client_max_body_size 1M;
    
    location / {
        root ./test_validation/www;
        index index.html;
        allow_methods GET POST DELETE;
    }
}
EOF

print_test "Serveur accepte un fichier de configuration"
timeout 3 ./$BINARY_NAME test_validation/test.conf &
WEBSERV_PID=$!
sleep 1

if kill -0 $WEBSERV_PID 2>/dev/null; then
    print_success "Serveur démarré avec fichier de config"
else
    print_critical "Serveur ne démarre pas avec fichier de config"
fi

# ============================================================================
# 3. TESTS HTTP OBLIGATOIRES (GET, POST, DELETE)
# ============================================================================
print_header "3. MÉTHODES HTTP OBLIGATOIRES"

print_test "Connexion TCP possible"
if timeout 5 bash -c "echo > /dev/tcp/localhost/8080" 2>/dev/null; then
    print_success "Connexion TCP établie"
else
    print_critical "Impossible de se connecter au serveur"
fi

print_test "Méthode GET obligatoire"
response=$(timeout 5 curl -s -w "%{http_code}" http://localhost:8080/ 2>/dev/null)
if echo "$response" | grep -q "200"; then
    print_success "GET fonctionne (code 200)"
elif echo "$response" | grep -qE "^[0-9]{3}$"; then
    print_error "GET retourne code: $(echo "$response" | tail -c 4)"
else
    print_critical "GET ne fonctionne pas du tout"
fi

print_test "Méthode POST obligatoire"
response=$(timeout 5 curl -s -w "%{http_code}" -X POST -d "test=data" http://localhost:8080/ 2>/dev/null)
if echo "$response" | grep -qE "^(200|201)"; then
    print_success "POST fonctionne (code: $(echo "$response" | tail -c 4))"
elif echo "$response" | grep -qE "^[0-9]{3}$"; then
    print_error "POST retourne code: $(echo "$response" | tail -c 4)"
else
    print_critical "POST ne fonctionne pas"
fi

print_test "Méthode DELETE obligatoire"
echo "test file" > test_validation/www/deleteme.txt
response=$(timeout 5 curl -s -w "%{http_code}" -X DELETE http://localhost:8080/deleteme.txt 2>/dev/null)
if echo "$response" | grep -qE "^(200|204)"; then
    print_success "DELETE fonctionne (code: $(echo "$response" | tail -c 4))"
elif echo "$response" | grep -qE "^(404|405)"; then
    print_success "DELETE traité correctement (code: $(echo "$response" | tail -c 4))"
else
    print_critical "DELETE ne fonctionne pas"
fi

# ============================================================================
# 4. TESTS DE ROBUSTESSE OBLIGATOIRES
# ============================================================================
print_header "4. ROBUSTESSE (OBLIGATOIRE)"

print_test "Serveur ne crash pas avec requête malformée"
echo -e "INVALID REQUEST\r\n\r\n" | timeout 3 nc localhost 8080 >/dev/null 2>&1
sleep 1
if kill -0 $WEBSERV_PID 2>/dev/null; then
    print_success "Serveur survit aux requêtes malformées"
else
    print_critical "Serveur crash avec requêtes malformées"
fi

print_test "Serveur gère les requêtes multiples"
for i in {1..5}; do
    timeout 2 curl -s http://localhost:8080/ >/dev/null &
done
wait
sleep 1
if kill -0 $WEBSERV_PID 2>/dev/null; then
    print_success "Serveur gère les requêtes multiples"
else
    print_critical "Serveur crash avec requêtes multiples"
fi

print_test "Gestion basique des erreurs 404"
response=$(timeout 5 curl -s -w "%{http_code}" http://localhost:8080/nonexistent 2>/dev/null)
if echo "$response" | grep -q "404"; then
    print_success "404 correctement retourné"
else
    print_error "404 mal géré (code: $(echo "$response" | tail -c 4))"
fi

# ============================================================================
# 5. TESTS CONFORMITÉ HTTP/1.1 BASIQUE
# ============================================================================
print_header "5. CONFORMITÉ HTTP/1.1 BASIQUE"

print_test "Réponse contient version HTTP/1.1"
response_headers=$(timeout 5 curl -s -I http://localhost:8080/ 2>/dev/null)
if echo "$response_headers" | grep -q "HTTP/1.1"; then
    print_success "Version HTTP/1.1 présente"
else
    print_error "Version HTTP/1.1 manquante ou incorrecte"
fi

print_test "Headers Content-Type et Content-Length"
if echo "$response_headers" | grep -qi "content-type" && echo "$response_headers" | grep -qi "content-length"; then
    print_success "Headers HTTP basiques présents"
else
    print_error "Headers HTTP basiques manquants"
fi

print_test "Headers Server et Connection"
if echo "$response_headers" | grep -qi "server:" && echo "$response_headers" | grep -qi "connection:"; then
    print_success "Headers Server et Connection présents"
else
    print_error "Headers Server et/ou Connection manquants"
fi

# ============================================================================
# 6. TESTS CRITIQUES DE SÉCURITÉ
# ============================================================================
print_header "6. SÉCURITÉ DE BASE"

print_test "Gestion des gros payloads"
large_response=$(timeout 10 curl -s -w "%{http_code}" -X POST --data-binary @<(head -c 100000 /dev/zero) http://localhost:8080/ 2>/dev/null)
if echo "$large_response" | grep -qE "(413|200)"; then
    print_success "Gros payloads gérés (code: $(echo "$large_response" | tail -c 4))"
    if kill -0 $WEBSERV_PID 2>/dev/null; then
        print_success "Serveur survit aux gros payloads"
    else
        print_critical "Serveur crash avec gros payload"
    fi
else
    print_error "Gros payloads mal gérés"
fi

print_test "Serveur ne consomme pas 100% CPU"
cpu_before=$(ps -p $WEBSERV_PID -o %cpu --no-headers 2>/dev/null || echo "0")
sleep 2
cpu_after=$(ps -p $WEBSERV_PID -o %cpu --no-headers 2>/dev/null || echo "0")
if [ "${cpu_after%.*}" -lt 50 ]; then
    print_success "Consommation CPU raisonnable"
else
    print_error "Consommation CPU élevée (${cpu_after}%)"
fi

# ============================================================================
# RÉSUMÉ FINAL
# ============================================================================
print_header "RÉSULTAT DE VALIDATION"

echo -e "📊 ${BLUE}Statistiques:${NC}"
echo -e "   Total des tests: ${TOTAL_TESTS}"
echo -e "   Tests réussis: ${GREEN}${PASSED_TESTS}${NC}"
echo -e "   Tests échoués: ${RED}${FAILED_TESTS}${NC}"
echo -e "   Erreurs critiques: ${RED}${CRITICAL_ERRORS}${NC}"

success_rate=$((PASSED_TESTS * 100 / TOTAL_TESTS))
echo -e "   Taux de réussite: ${BLUE}${success_rate}%${NC}"

echo ""
if [ $CRITICAL_ERRORS -eq 0 ] && [ $success_rate -ge 85 ]; then
    echo -e "${GREEN}🎉 VALIDATION RÉUSSIE !${NC}"
    echo -e "${GREEN}Votre serveur respecte les exigences minimales du sujet.${NC}"
    echo -e "${GREEN}Vous pouvez soumettre votre projet en toute confiance !${NC}"
    exit 0
elif [ $CRITICAL_ERRORS -eq 0 ] && [ $success_rate -ge 75 ]; then
    echo -e "${YELLOW}⚠️  VALIDATION PARTIELLE${NC}"
    echo -e "${YELLOW}Votre serveur fonctionne mais il y a des problèmes mineurs.${NC}"
    echo -e "${YELLOW}Corrigez les erreurs non-critiques avant soumission.${NC}"
    exit 0
else
    echo -e "${RED}❌ VALIDATION ÉCHOUÉE${NC}"
    echo -e "${RED}Votre serveur a des problèmes critiques.${NC}"
    echo -e "${RED}NE SOUMETTEZ PAS en l'état.${NC}"
    echo ""
    echo -e "${YELLOW}Problèmes à corriger en priorité:${NC}"
    if [ $CRITICAL_ERRORS -gt 0 ]; then
        echo -e "  - ${RED}${CRITICAL_ERRORS} erreur(s) critique(s)${NC}"
    fi
    if [ $success_rate -lt 75 ]; then
        echo -e "  - Taux de réussite trop faible (${success_rate}% < 75%)"
    fi
    exit 1
fi