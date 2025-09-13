document.addEventListener('DOMContentLoaded', () => {
    const ctx = document.getElementById('sensorChart').getContext('2d');
    let chart;

    const currentTempElem = document.getElementById('current-temp');
    const currentHumidityElem = document.getElementById('current-humidity');
    const lastUpdateElem = document.getElementById('last-update');
    const historyBodyElem = document.getElementById('history-body');

    function initializeChart() {
        chart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [
                    {
                        label: 'Temperatura (°C)',
                        data: [],
                        borderColor: 'rgba(255, 99, 132, 1)',
                        backgroundColor: 'rgba(255, 99, 132, 0.2)',
                        yAxisID: 'y',
                        tension: 0.1
                    },
                    {
                        label: 'Umidade (%)',
                        data: [],
                        borderColor: 'rgba(54, 162, 235, 1)',
                        backgroundColor: 'rgba(54, 162, 235, 0.2)',
                        yAxisID: 'y1',
                        tension: 0.1
                    }
                ]
            },
            options: {
                responsive: true,
                scales: {
                    x: {
                        type: 'time',
                        time: {
                            unit: 'minute',
                            tooltipFormat: 'dd/MM/yyyy HH:mm:ss'
                        },
                        title: { display: true, text: 'Horário' }
                    },
                    y: {
                        type: 'linear',
                        display: true,
                        position: 'left',
                        title: { display: true, text: 'Temperatura (°C)' }
                    },
                    y1: {
                        type: 'linear',
                        display: true,
                        position: 'right',
                        title: { display: true, text: 'Umidade (%)' },
                        grid: { drawOnChartArea: false }
                    }
                }
            }
        });
    }

    async function fetchData() {
        try {
            const response = await fetch('/api/data');
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            const data = await response.json();

            updatePage(data);
        } catch (error) {
            console.error("Falha ao buscar dados:", error);
            lastUpdateElem.textContent = 'Erro ao carregar';
        }
    }

    function updatePage(data) {
        if (!data || data.length === 0) {
            return;
        }

        // Os dados vêm do mais novo para o mais antigo, então pegamos o primeiro [0]
        const latest = data[0];
        const latestDate = new Date(latest.created_at);
        currentTempElem.textContent = `${Number(latest.temperature).toFixed(1)} °C`;
        currentHumidityElem.textContent = `${Number(latest.humidity).toFixed(1)} %`;
        lastUpdateElem.textContent = latestDate.toLocaleString('pt-BR');

        // Inverte os dados para o gráfico (do mais antigo para o mais novo)
        const reversedData = [...data].reverse();

        // Atualiza dados do gráfico
        chart.data.labels = reversedData.map(d => new Date(d.created_at));
        chart.data.datasets[0].data = reversedData.map(d => d.temperature);
        chart.data.datasets[1].data = reversedData.map(d => d.humidity);
        chart.update();

        // Atualiza a tabela
        historyBodyElem.innerHTML = ''; // Limpa a tabela
        data.forEach(reading => {
            const row = document.createElement('tr');
            const date = new Date(reading.created_at);
            row.innerHTML = `
                <td>${date.toLocaleString('pt-BR')}</td>
                <td>${Number(reading.temperature).toFixed(1)} °C</td>
                <td>${Number(reading.humidity).toFixed(1)} %</td>
            `;
            historyBodyElem.appendChild(row);
        });
    }

    initializeChart();
    fetchData(); // Busca os dados na primeira vez
    
    // Atualiza os dados a cada 30 segundos
    setInterval(fetchData, 30000);
});
