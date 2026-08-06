import React, { useState, useEffect } from 'react'

function App() {
  const [comando, setComando] = useState('')
  const [respuesta, setRespuesta] = useState('')
  const [cargando, setCargando] = useState(false)
  const [conectado, setConectado] = useState(false)

  const probarConexion = async () => {
    try {
      const response = await fetch('/api/ping')
      const data = await response.json()
      setConectado(true)
      setRespuesta(`✅ Conectado: ${data.message}`)
    } catch (error) {
      setConectado(false)
      setRespuesta('❌ Error de conexión: No se pudo conectar al servidor')
      console.error('Error:', error)
    }
  }

  const enviarComando = async () => {
    if (!comando.trim()) {
      setRespuesta('⚠️ Por favor, ingresa un comando')
      return
    }

    setCargando(true)
    setRespuesta('⏳ Procesando...')

    try {
      const response = await fetch('/api/comando', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ comando: comando }),
      })

      const data = await response.json()
      
      if (data.success) {
        setRespuesta(`✅ ${data.resultado}`)
      } else {
        setRespuesta(`❌ Error: ${data.error || 'Error desconocido'}`)
      }
    } catch (error) {
      setRespuesta('❌ Error de conexión con el servidor')
      console.error('Error:', error)
    }

    setCargando(false)
  }

  useEffect(() => {
    probarConexion()
  }, [])

  return (
    <div className="app">
      <h1>🚀 Analizador de Comandos EXT2</h1>
      
      <div className="conexion-status">
        <span className={conectado ? 'conectado' : 'desconectado'}>
          {conectado ? '🟢 Conectado al servidor' : '🔴 Desconectado'}
        </span>
        <button onClick={probarConexion} className="btn-test">
          Probar conexión
        </button>
      </div>

      <div className="input-area">
        <label htmlFor="comando">Ingresa un comando:</label>
        <div className="input-group">
          <textarea
            id="comando"
            value={comando}
            onChange={(e) => setComando(e.target.value)}
            placeholder="Ej: mkdisk -size=10 -path=/home/Disco1.mia"
            rows="3"
          />
          <button 
            onClick={enviarComando} 
            disabled={cargando || !conectado}
            className="btn-enviar"
          >
            {cargando ? '⏳ Enviando...' : '▶️ Ejecutar'}
          </button>
        </div>
      </div>

      <div className="output-area">
        <h3>📋 Resultado:</h3>
        <div className="output-box">
          {respuesta || 'Esperando comando...'}
        </div>
      </div>
    </div>
  )
}

export default App