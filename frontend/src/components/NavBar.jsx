import React from 'react';
import { Link, useNavigate } from 'react-router-dom';
import { useSession } from '../context/SessionContext';

export default function NavBar() {
  const { session, logout } = useSession();
  const navigate = useNavigate();

  const handleLogout = async () => {
    await logout();
    navigate('/');
  };

  return (
    <nav style={{
      background: '#1e293b',
      padding: '1rem 2rem',
      color: 'white',
      display: 'flex',
      justifyContent: 'space-between',
      alignItems: 'center',
      marginBottom: '2rem'
    }}>
      <div style={{ display: 'flex', gap: '1.5rem' }}>
        <Link to="/" style={{ color: 'white', textDecoration: 'none', fontWeight: 'bold' }}>
          🏠 Home
        </Link>
        <Link to="/visualizador" style={{ color: 'white', textDecoration: 'none' }}>
          📁 Visualizador
        </Link>
        {session?.active && (
          <Link to="/journal" style={{ color: 'white', textDecoration: 'none' }}>
            📓 Journal
          </Link>
        )}
      </div>
      
      <div>
        {session?.active ? (
          <div style={{ display: 'flex', gap: '1rem', alignItems: 'center' }}>
            <span>👤 {session.user} ({session.id})</span>
            <button 
              onClick={handleLogout}
              style={{
                background: '#dc2626',
                color: 'white',
                border: 'none',
                padding: '0.5rem 1rem',
                borderRadius: '4px',
                cursor: 'pointer'
              }}
            >
              Cerrar Sesión
            </button>
          </div>
        ) : (
          <Link to="/login" style={{
            background: '#16a34a',
            color: 'white',
            padding: '0.5rem 1rem',
            borderRadius: '4px',
            textDecoration: 'none'
          }}>
            Iniciar Sesión
          </Link>
        )}
      </div>
    </nav>
  );
}