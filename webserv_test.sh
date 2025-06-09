#!/bin/bash

# Couleurs pour l'affichage
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Variables globales
WEBSERV_PID=""
TEST_PORT=8080
TEST_DIR="./test_webserv"
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Fonction d'affichage
print_header() {
    echo -e "${BLUE}================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}================================${NC}"
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

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

# Fonction de nettoyage
cleanup() {
    echo -e "\n${BLUE}Nettoyage...${NC}"
    if [ ! -z "$WEBSERV_PID" ]; then
        kill -9 $WEBSERV_PID 2>/dev/null
        wait $WEBSERV_PID 2>/dev/null
    fi
    rm -rf $TEST_DIR
    echo "Nettoyage terminé"
}

# Piège pour nettoyer à la fin
trap cleanup EXIT

# Fonction pour démarrer le serveur
start_server() {
    print_header "DÉMARRAGE DU SERVEUR"
    
    # Créer le répertoire de test
    mkdir -p $TEST_DIR/www
    mkdir -p $TEST_DIR/uploads
    mkdir -p $TEST_DIR/cgi-bin
    
    # Créer un fichier de configuration de test
    cat > $TEST_DIR/webserv.conf << 'EOF'
server {
    listen 8080;
    server_name localhost;
    client_max_body_size 1M;
    error_page 404 /errors/404.html;
    error_page 500 /errors/500.html;
    
    location / {
        root ./test_webserv/www;
        index index.html;
        allow_methods GET POST DELETE;
        directory_listing on;
    }
    
    location /upload {
        root ./test_webserv/www;
        allow_methods GET POST;
        upload_store ./test_webserv/uploads;
    }
    
    location /cgi-bin {
        root ./test_webserv;
        allow_methods GET POST;
        cgi_pass ./test_webserv/cgi-bin/;
        cgi_extensions .py .sh;
    }
}
EOF

    # Créer des fichiers de test
    cat > $TEST_DIR/www/index.html << 'EOF'
<!DOCTYPE html>
<html>
<head><title>Test Webserv</title></head>
<body>
    <h1>Bienvenue sur Webserv</h1>
    <p>Serveur fonctionnel !</p>
    <a href="/test.html">Page de test</a>
</body>
</html>
EOF

    cat > $TEST_DIR/www/test.html << 'EOF'
<!DOCTYPE html>
<html>
<head><title>Page de test</title></head>
<body><h1>Page de test</h1></body>
</html>
EOF

    # Créer un script CGI de test
    cat > $TEST_DIR/cgi-bin/test.py << 'EOF'
#!/usr/bin/env python3
import os
import sys

print("Content-Type: text/html")
print()
print("<html><body>")
print("<h1>CGI Test</h1>")
print(f"<p>REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'Not set')}</p>")
print(f"<p>QUERY_STRING: {os.environ.get('QUERY_STRING', 'Not set')}</p>")
print(f"<p>CONTENT_LENGTH: {os.environ.get('CONTENT_LENGTH', 'Not set')}</p>")
print("</body></html>")
EOF

    chmod +x $TEST_DIR/cgi-bin/test.py

    # Vérifier que le binaire webserv existe
    if [ ! -f "./webserv" ]; then
        print_error "Le binaire ./webserv n'existe pas. Compilez d'abord votre projet."
        exit 1
    fi

    # Démarrer le serveur
    ./webserv $TEST_DIR/webserv.conf &
    WEBSERV_PID=$!
    
    sleep 2
    
    # Vérifier que le serveur est démarré
    if ! kill -0 $WEBSERV_PID 2>/dev/null; then
        print_error "Le serveur ne s'est pas démarré correctement"
        exit 1
    fi
    
    print_success "Serveur démarré (PID: $WEBSERV_PID)"
}

# Tests de compilation
test_compilation() {
    print_header "TESTS DE COMPILATION"
    
    print_test "Compilation du projet"
    if make re > /dev/null 2>&1; then
        print_success "Compilation réussie"
    else
        print_error "Échec de compilation"
        make re
        return 1
    fi
    
    print_test "Vérification des warnings"
    if make 2>&1 | grep -q "warning"; then
        print_warning "Des warnings ont été détectés"
        make 2>&1 | grep "warning"
    else
        print_success "Aucun warning"
    fi
}

# Tests de connectivité de base
test_basic_connectivity() {
    print_header "TESTS DE CONNECTIVITÉ DE BASE"
    
    print_test "Connexion au serveur"
    if curl -s --connect-timeout 5 http://localhost:$TEST_PORT/ > /dev/null; then
        print_success "Connexion établie"
    else
        print_error "Impossible de se connecter au serveur"
        return 1
    fi
    
    print_test "Réponse GET basique"
    response=$(curl -s http://localhost:$TEST_PORT/)
    if echo "$response" | grep -q "Bienvenue sur Webserv"; then
        print_success "Page d'accueil accessible"
    else
        print_error "Page d'accueil incorrecte"
        echo "Réponse reçue: $response"
    fi
}

# Tests des méthodes HTTP
test_http_methods() {
    print_header "TESTS DES MÉTHODES HTTP"
    
    print_test "Méthode GET"
    status=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:$TEST_PORT/test.html)
    if [ "$status" = "200" ]; then
        print_success "GET fonctionne (code: $status)"
    else
        print_error "GET échoue (code: $status)"
    fi
    
    print_test "Méthode POST"
    status=$(curl -s -o /dev/null -w "%{http_code}" -X POST -d "test=data" http://localhost:$TEST_PORT/upload)
    if [ "$status" = "200" ] || [ "$status" = "201" ]; then
        print_success "POST fonctionne (code: $status)"
    else
        print_error "POST échoue (code: $status)"
    fi
    
    print_test "Méthode DELETE"
    # Créer un fichier à supprimer
    echo "test file" > $TEST_DIR/www/delete_me.txt
    status=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE http://localhost:$TEST_PORT/delete_me.txt)
    if [ "$status" = "200" ] || [ "$status" = "204" ]; then
        print_success "DELETE fonctionne (code: $status)"
    else
        print_error "DELETE échoue (code: $status)"
    fi
    
    print_test "Méthode non autorisée"
    status=$(curl -s -o /dev/null -w "%{http_code}" -X PUT http://localhost:$TEST_PORT/)
    if [ "$status" = "405" ]; then
        print_success "Méthode PUT correctement rejetée (code: $status)"
    else
        print_error "Méthode PUT pas correctement gérée (code: $status)"
    fi
}

# Tests des codes d'erreur
test_error_codes() {
    print_header "TESTS DES CODES D'ERREUR"
    
    print_test "404 Not Found"
    status=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:$TEST_PORT/nonexistent.html)
    if [ "$status" = "404" ]; then
        print_success "404 correctement retourné"
    else
        print_error "404 attendu, reçu: $status"
    fi
    
    print_test "413 Payload Too Large"
    # Créer un gros fichier (plus de 1MB)
    large_data=$(python3 -c "print('x' * 2000000)")
    status=$(curl -s -o /dev/null -w "%{http_code}" -X POST -d "$large_data" http://localhost:$TEST_PORT/upload)
    if [ "$status" = "413" ]; then
        print_success "413 correctement retourné pour gros payload"
    else
        print_warning "413 attendu pour gros payload, reçu: $status"
    fi
}

# Tests CGI
test_cgi() {
    print_header "TESTS CGI"
    
    print_test "Script CGI Python"
    response=$(curl -s http://localhost:$TEST_PORT/cgi-bin/test.py)
    if echo "$response" | grep -q "CGI Test"; then
        print_success "CGI Python fonctionne"
    else
        print_error "CGI Python ne fonctionne pas"
        echo "Réponse: $response"
    fi
    
    print_test "Variables d'environnement CGI"
    response=$(curl -s http://localhost:$TEST_PORT/cgi-bin/test.py?param=value)
    if echo "$response" | grep -q "QUERY_STRING.*param=value"; then
        print_success "QUERY_STRING correctement transmis"
    else
        print_error "QUERY_STRING mal transmis"
        echo "Réponse: $response"
    fi
    
    print_test "CGI avec POST"
    response=$(curl -s -X POST -d "test=data" http://localhost:$TEST_PORT/cgi-bin/test.py)
    if echo "$response" | grep -q "REQUEST_METHOD.*POST"; then
        print_success "REQUEST_METHOD POST correctement transmis"
    else
        print_error "REQUEST_METHOD POST mal transmis"
    fi
}

# Tests de robustesse
test_robustness() {
    print_header "TESTS DE ROBUSTESSE"
    
    print_test "Multiples connexions simultanées"
    for i in {1..10}; do
        curl -s http://localhost:$TEST_PORT/ > /dev/null &
    done
    wait
    
    if kill -0 $WEBSERV_PID 2>/dev/null; then
        print_success "Serveur survit aux connexions multiples"
    else
        print_error "Serveur crash avec connexions multiples"
        return 1
    fi
    
    print_test "Headers malformés"
    # Test avec des headers invalides
    response=$(echo -e "GET / HTTP/1.1\r\nInvalid Header\r\n\r\n" | nc localhost $TEST_PORT 2>/dev/null)
    if kill -0 $WEBSERV_PID 2>/dev/null; then
        print_success "Serveur survit aux headers malformés"
    else
        print_error "Serveur crash avec headers malformés"
    fi
    
    print_test "Requête incomplète"
    # Connexion qui se ferme brutalement
    timeout 2 bash -c "echo 'GET /' | nc localhost $TEST_PORT" > /dev/null 2>&1
    sleep 1
    if kill -0 $WEBSERV_PID 2>/dev/null; then
        print_success "Serveur survit aux requêtes incomplètes"
    else
        print_error "Serveur crash avec requêtes incomplètes"
    fi
}

# Tests de conformité HTTP
test_http_compliance() {
    print_header "TESTS DE CONFORMITÉ HTTP"
    
    print_test "Headers HTTP obligatoires"
    headers=$(curl -s -I http://localhost:$TEST_PORT/)
    
    if echo "$headers" | grep -qi "content-type"; then
        print_success "Header Content-Type présent"
    else
        print_error "Header Content-Type manquant"
    fi
    
    if echo "$headers" | grep -qi "content-length"; then
        print_success "Header Content-Length présent"
    else
        print_error "Header Content-Length manquant"
    fi
    
    print_test "Version HTTP"
    if echo "$headers" | grep -q "HTTP/1.1"; then
        print_success "Version HTTP/1.1 correcte"
    else
        print_error "Version HTTP incorrecte"
        echo "Headers reçus: $headers"
    fi
}

# Test de détection des problèmes spécifiques
test_specific_issues() {
    print_header "DÉTECTION DES PROBLÈMES SPÉCIFIQUES"
    
    print_test "Fonction trim() dupliquée"
    if grep -r "std::string trim(" . --include="*.cpp" --include="*.hpp" | wc -l | grep -q "^[2-9]"; then
        print_error "Fonction trim() définie plusieurs fois (problème de compilation)"
        grep -r "std::string trim(" . --include="*.cpp" --include="*.hpp"
    else
        print_success "Pas de duplication de la fonction trim()"
    fi
    
    print_test "Variables CGI correctes"
    response=$(curl -s "http://localhost:$TEST_PORT/cgi-bin/test.py?test=123")
    if echo "$response" | grep -q "QUERY_STRING.*test=123"; then
        print_success "QUERY_STRING correctement formaté"
    else
        print_error "QUERY_STRING mal formaté (manque le = ?)"
    fi
    
    print_test "Gestion des file descriptors"
    initial_fds=$(lsof -p $WEBSERV_PID 2>/dev/null | wc -l)
    for i in {1..5}; do
        curl -s http://localhost:$TEST_PORT/ > /dev/null
    done
    sleep 1
    final_fds=$(lsof -p $WEBSERV_PID 2>/dev/null | wc -l)
    
    if [ $final_fds -le $((initial_fds + 2)) ]; then
        print_success "Pas de fuite de file descriptors détectée"
    else
        print_warning "Possible fuite de file descriptors ($initial_fds -> $final_fds)"
    fi
}

# Test avec navigateur
test_browser_compatibility() {
    print_header "TEST DE COMPATIBILITÉ NAVIGATEUR"
    
    print_test "Simulation requête navigateur"
    response=$(curl -s -H "User-Agent: Mozilla/5.0" -H "Accept: text/html" http://localhost:$TEST_PORT/)
    if [ $? -eq 0 ] && echo "$response" | grep -q "html"; then
        print_success "Compatible avec les requêtes navigateur"
    else
        print_error "Problème avec les requêtes navigateur"
    fi
}

# Fonction principale
main() {
    echo -e "${BLUE}"
    echo "╔══════════════════════════════════════╗"
    echo "║        TEST WEBSERV COMPLET          ║"
    echo "╚══════════════════════════════════════╝"
    echo -e "${NC}\n"
    
    # Vérifier les dépendances
    if ! command -v curl &> /dev/null; then
        print_error "curl n'est pas installé"
        exit 1
    fi
    
    if ! command -v nc &> /dev/null; then
        print_error "netcat n'est pas installé"
        exit 1
    fi
    
    # Lancer les tests
    test_compilation || exit 1
    start_server
    test_basic_connectivity || exit 1
    test_http_methods
    test_error_codes
    test_cgi
    test_robustness
    test_http_compliance
    test_specific_issues
    test_browser_compatibility
    
    # Résumé final
    print_header "RÉSUMÉ DES TESTS"
    echo -e "Total des tests: ${BLUE}$TOTAL_TESTS${NC}"
    echo -e "Tests réussis: ${GREEN}$PASSED_TESTS${NC}"
    echo -e "Tests échoués: ${RED}$FAILED_TESTS${NC}"
    
    if [ $FAILED_TESTS -eq 0 ]; then
        echo -e "\n${GREEN}🎉 Tous les tests sont passés ! Votre serveur semble fonctionnel.${NC}"
        exit 0
    else
        echo -e "\n${RED}⚠️  Certains tests ont échoué. Vérifiez les erreurs ci-dessus.${NC}"
        exit 1
    fi
}

# Lancer le script
main "$@"