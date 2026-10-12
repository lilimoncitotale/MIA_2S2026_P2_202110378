import React, { useState, useEffect } from 'react';
import { useSearchParams } from 'react-router-dom';
import { listarCarpeta, leerArchivo } from '../api';

export default function FileExplorer() {
  const [searchParams] = useSearchParams();
  const id = searchParams.get('id');
  
  const [path, setPath] = useState('/');
  const [files, setFiles] = useState([]);
  const [loading, setLoading] = useState(false);
  const [archivoAbierto, setArchivoAbierto] = useState(null);
  const [contenido, setContenido] = useState('');

  const cargarCarpeta = async (p) => {
    setLoading(true);
    try {
      const data = await listarCarpeta(id, p);
      setFiles(data.files || []);
      setPath(p);
    } catch (e) {
      console.error(e);
    }
    setLoading(false);
  };

  useEffect(() => {
    if (id) cargarCarpeta('/');
  }, [id]);

  const abrirArchivo = async (file) => {
    if (file.type === 'folder') {
      const newPath = path === '/' ? `/${file.name}` : `${path}/${file.name}`;
      cargarCarpeta(newPath);
    } else {
      try {
        const fullPath = path === '/' ? `/${file.name}` : `${path}/${file.name}`;
        const data = await leerArchivo(id, fullPath);
        if (data.content !== undefined) {
          setContenido(data.content);
          setArchivoAbierto(file.name);
        } else {
          alert('No se pudo leer el archivo');
        }
      } catch (e) {
        alert('Error al leer archivo');
      }
    }
  };

  const irAtras = () => {
    if (path === '/') return;
    const parent = path.substring(0, path.lastIndexOf('/')) || '/';
    cargarCarpeta(parent);
  };

  return (
    <div style={{ padding: '2rem' }}>
      <h2>📁 Visualizador de Archivos</h2>
      <p style={{ color: '#666' }}>Partición: {id}</p>

      {/* Barra de ruta */}
      <div style={{ display: 'flex', gap: '0.5rem', marginTop: '1rem' }}>
        <button onClick={irAtras} disabled={path === '/'}
          style={{ padding: '0.5rem 1rem', background: '#64748b', color: 'white', border: 'none', borderRadius: '4px', cursor: path === '/' ? 'not-allowed' : 'pointer' }}>
          ← Atrás
        </button>
        <input
          type="text"
          value={path}
          onChange={(e) => setPath(e.target.value)}
          onKeyDown={(e) => e.key === 'Enter' && cargarCarpeta(path)}
          style={{ flex: 1, padding: '0.5rem', background: '#f1f5f9', border: '1px solid #ccc', borderRadius: '4px', fontFamily: 'monospace' }}
        />
        <button onClick={() => cargarCarpeta(path)}
          style={{ padding: '0.5rem 1rem', background: '#2563eb', color: 'white', border: 'none', borderRadius: '4px', cursor: 'pointer' }}>
          Ir
        </button>
      </div>

      {loading ? (
        <p>Cargando...</p>
      ) : (
        <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(150px, 1fr))', gap: '1rem', marginTop: '2rem' }}>
          {files.map((file, idx) => (
            <div
              key={idx}
              onClick={() => abrirArchivo(file)}
              style={{
                padding: '1rem',
                background: 'white',
                border: '1px solid #ddd',
                borderRadius: '8px',
                cursor: 'pointer',
                textAlign: 'center'
              }}
            >
              <div style={{ fontSize: '2.5rem' }}>
                {file.type === 'folder' ? '📁' : '📄'}
              </div>
              <div style={{ fontWeight: 'bold', fontSize: '0.9rem', wordBreak: 'break-word' }}>
                {file.name}
              </div>
              <div style={{ fontSize: '0.75rem', color: '#666', marginTop: '0.25rem' }}>
                {file.perm} · {file.size} B
              </div>
            </div>
          ))}
          {files.length === 0 && (
            <p style={{ gridColumn: '1 / -1', textAlign: 'center', color: '#666' }}>
              Carpeta vacía
            </p>
          )}
        </div>
      )}

      {/* Modal de archivo */}
      {archivoAbierto && (
        <div style={{
          position: 'fixed',
          top: 0, left: 0, right: 0, bottom: 0,
          background: 'rgba(0,0,0,0.5)',
          display: 'flex',
          justifyContent: 'center',
          alignItems: 'center',
          zIndex: 1000
        }} onClick={() => setArchivoAbierto(null)}>
          <div style={{
            background: 'white',
            padding: '2rem',
            borderRadius: '8px',
            maxWidth: '600px',
            width: '90%',
            maxHeight: '80vh',
            overflow: 'auto'
          }} onClick={(e) => e.stopPropagation()}>
            <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '1rem' }}>
              <h3>📄 {archivoAbierto}</h3>
              <button onClick={() => setArchivoAbierto(null)}
                style={{ background: '#dc2626', color: 'white', border: 'none', padding: '0.5rem 1rem', borderRadius: '4px', cursor: 'pointer' }}>
                Cerrar
              </button>
            </div>
            <pre style={{ background: '#f1f5f9', padding: '1rem', borderRadius: '4px', whiteSpace: 'pre-wrap', wordBreak: 'break-word' }}>
              {contenido}
            </pre>
          </div>
        </div>
      )}
    </div>
  );
}