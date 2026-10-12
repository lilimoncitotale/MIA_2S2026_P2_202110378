const API_URL = 'http://localhost:8080/api';

// Helper genérico
async function fetchJSON(url, options = {}) {
  const response = await fetch(url, options);
  if (!response.ok) {
    throw new Error(`HTTP ${response.status}`);
  }
  return response.json();
}

// ============================================
// DISCOS
// ============================================
export async function listarDiscos() {
  return fetchJSON(`${API_URL}/discos`);
}

export async function infoDisco(path) {
  return fetchJSON(`${API_URL}/disco?path=${encodeURIComponent(path)}`);
}

// ============================================
// PARTICIÓN
// ============================================
export async function infoParticion(id) {
  return fetchJSON(`${API_URL}/particion?id=${encodeURIComponent(id)}`);
}

// ============================================
// SISTEMA DE ARCHIVOS
// ============================================
export async function listarCarpeta(id, path = '/') {
  return fetchJSON(`${API_URL}/fs/ls?id=${encodeURIComponent(id)}&path=${encodeURIComponent(path)}`);
}

export async function leerArchivo(id, path) {
  return fetchJSON(`${API_URL}/fs/file?id=${encodeURIComponent(id)}&path=${encodeURIComponent(path)}`);
}

// ============================================
// SESIÓN
// ============================================
export async function loginGrafico(id, user, pass) {
  return fetchJSON(`${API_URL}/login`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ id, user, pass })
  });
}

export async function logoutGrafico() {
  return fetchJSON(`${API_URL}/logout`, { method: 'POST' });
}

export async function estadoSesion() {
  return fetchJSON(`${API_URL}/session`);
}

// ============================================
// JOURNAL
// ============================================
export async function listarJournal(id) {
  return fetchJSON(`${API_URL}/journaling?id=${encodeURIComponent(id)}`);
}