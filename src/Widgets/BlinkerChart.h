#ifndef BLINKER_CHART_H
#define BLINKER_CHART_H

// #if defined(BLINKER_WIFI)

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

typedef void (*blinker_chart_callback_t)(void);

void (*_blinker_chart_sync_callback)(void) = nullptr;

class BlinkerChart;

class BlinkerChartManager {
private:
    BlinkerChartManager() : chartCount(0), dataStorageSetup(false) {
        _blinker_chart_sync_callback = []() {
            BlinkerChartManager::getInstance().syncAllChartsMode();
        };
    }
    
    BlinkerChart* charts[BLINKER_MAX_WIDGET_SIZE];
    uint8_t chartCount;
    bool dataStorageSetup;
    
public:
    static BlinkerChartManager& getInstance() {
        static BlinkerChartManager instance;
        return instance;
    }
    
    void registerChart(BlinkerChart* chart);
    void setupDataStorage(uint32_t interval_seconds = 60);
    static void globalDataCallback();
    
    void syncAllChartsMode();
    
    void triggerRealtimeCallback(const char* chartName);
};

class BlinkerChart
{
    public :
        BlinkerChart(const char* _name)
            : chartName(_name)
            , isRealtimeMode(false)
            , lastUploadTime(0)
            , uploadInterval(5000)
            , chartId(255)
            , _realtimeCallback(nullptr)
        {
            // 支持WiFi和BLE模式的图表注册
            #if defined(BLINKER_WIFI) || defined(BLINKER_BLE)
            chartId = Blinker.registerChart(_name, nullptr);
            #endif
            
            BlinkerChartManager::getInstance().registerChart(this);
        }

        ~BlinkerChart() {}

        BlinkerChart& upload(const char* dataKey, uint8_t value)
        {
            syncRealtimeMode();
            
            if (isRealtimeMode) {
                uploadRealtime(dataKey, value);
            } else {
                uploadHistorical(dataKey, value);
            }
            return *this;
        }
        
        BlinkerChart& upload(const char* dataKey, int8_t value)
        {
            syncRealtimeMode();
            
            if (isRealtimeMode) {
                uploadRealtime(dataKey, value);
            } else {
                uploadHistorical(dataKey, value);
            }
            return *this;
        }
        
        BlinkerChart& upload(const char* dataKey, uint16_t value)
        {
            syncRealtimeMode();
            
            if (isRealtimeMode) {
                uploadRealtime(dataKey, value);
            } else {
                uploadHistorical(dataKey, value);
            }
            return *this;
        }
        
        BlinkerChart& upload(const char* dataKey, int16_t value)
        {
            syncRealtimeMode();
            
            if (isRealtimeMode) {
                uploadRealtime(dataKey, value);
            } else {
                uploadHistorical(dataKey, value);
            }
            return *this;
        }
        
        BlinkerChart& upload(const char* dataKey, uint32_t value)
        {
            syncRealtimeMode();
            
            if (isRealtimeMode) {
                uploadRealtime(dataKey, value);
            } else {
                uploadHistorical(dataKey, value);
            }
            return *this;
        }
        
        BlinkerChart& upload(const char* dataKey, int32_t value)
        {
            syncRealtimeMode();
            
            if (isRealtimeMode) {
                uploadRealtime(dataKey, value);
            } else {
                uploadHistorical(dataKey, value);
            }
            return *this;
        }
        
        BlinkerChart& upload(const char* dataKey, float value)
        {
            syncRealtimeMode();
            
            if (isRealtimeMode) {
                uploadRealtime(dataKey, value);
            } else {
                uploadHistorical(dataKey, value);
            }
            return *this;
        }
        
        BlinkerChart& upload(const char* dataKey, double value)
        {
            syncRealtimeMode();
            
            if (isRealtimeMode) {
                uploadRealtime(dataKey, value);
            } else {
                uploadHistorical(dataKey, value);
            }
            return *this;
        }
        
        BlinkerChart& upload(const char* dataKey, int value)
        {
            return upload(dataKey, static_cast<int32_t>(value));
        }
        
        BlinkerChart& upload(const char* dataKey, unsigned int value)
        {
            return upload(dataKey, static_cast<uint32_t>(value));
        }

        BlinkerChart& interval(uint32_t _interval)
        {
            uploadInterval = _interval;
            // 支持WiFi和BLE模式的间隔设置
            #if defined(BLINKER_WIFI) || defined(BLINKER_BLE)
            if (chartId != 255) {
                Blinker.setChartInterval(chartId, _interval);
            }
            #endif
            return *this;
        }

        void attach(blinker_chart_callback_t newFunction, uint32_t interval_seconds = 60)
        {
            _realtimeCallback = newFunction;
            
            if (newFunction != nullptr) {
                #if defined(BLINKER_WIFI)
                BlinkerChartManager::getInstance().setupDataStorage(interval_seconds);
                BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] "), chartName, BLINKER_F(" attached callback with interval: "), interval_seconds, BLINKER_F(" seconds"));
                #else
                BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Data storage not supported in BLE mode, only realtime callbacks available"));
                #endif
            }
        }

        bool isRealtime() const { return isRealtimeMode; }

        BlinkerChart& setRealtimeMode(bool realtime)
        {
            isRealtimeMode = realtime;
            // 支持WiFi和BLE模式的实时模式设置
            #if defined(BLINKER_WIFI) || defined(BLINKER_BLE)
            if (chartId != 255) {
                Blinker.setChartRealtimeMode(chartId, realtime);
            }
            #endif
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] "), chartName, 
                           BLINKER_F(" mode switched to: "), 
                           realtime ? BLINKER_F("realtime") : BLINKER_F("historical"));
            return *this;
        }

        const char* getName() const { return chartName; }
        
        uint32_t getUploadInterval() const { return uploadInterval; }
        
        void forceSyncRealtimeMode() { syncRealtimeMode(); }

        void triggerRealtimeCallback()
        {
            if (_realtimeCallback && isRealtimeMode) {
                BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Triggering realtime callback for: "), chartName);
                _realtimeCallback();
            }
        }
        
        void triggerDataCallback()
        {
            if (_realtimeCallback) {
                BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Triggering data callback for: "), chartName);
                _realtimeCallback();
            }
        }

    private :
        const char*    chartName;          // 图表名称
        bool           isRealtimeMode;      // 当前模式：true=实时，false=历史
        uint32_t       lastUploadTime;     // 上次上传时间
        uint32_t       uploadInterval;     // 上传间隔（毫秒）
        uint8_t        chartId;            // 图表在协议层的ID
        blinker_chart_callback_t _realtimeCallback;  // 实时数据回调函数

        void syncRealtimeMode()
        {
            // 支持WiFi和BLE模式的实时模式同步
            #if defined(BLINKER_WIFI) || defined(BLINKER_BLE)
            if (chartId != 255) {
                bool protocolMode = Blinker.isChartRealtime(chartName);
                if (protocolMode != isRealtimeMode) {
                    isRealtimeMode = protocolMode;
                    BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Synced mode from protocol: "), 
                                   chartName, BLINKER_F(" -> "), 
                                   isRealtimeMode ? BLINKER_F("realtime") : BLINKER_F("historical"));
                }
            }
            #endif
        }

        void uploadHistorical(const char* dataKey, uint8_t value)
        {
            // 历史数据上传仅支持WiFi模式（需要云端存储）
            #if defined(BLINKER_WIFI)
            Blinker.chartDataUpload(dataKey, value);
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Historical data buffered: "), dataKey, BLINKER_F("="), value);
            #else
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Historical data upload not supported in BLE mode"));
            #endif
        }
        
        void uploadHistorical(const char* dataKey, int8_t value)
        {
            // 历史数据上传仅支持WiFi模式（需要云端存储）
            #if defined(BLINKER_WIFI)
            Blinker.chartDataUpload(dataKey, value);
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Historical data buffered: "), dataKey, BLINKER_F("="), value);
            #else
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Historical data upload not supported in BLE mode"));
            #endif
        }
        
        void uploadHistorical(const char* dataKey, uint16_t value)
        {
            // 历史数据上传仅支持WiFi模式（需要云端存储）
            #if defined(BLINKER_WIFI)
            Blinker.chartDataUpload(dataKey, value);
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Historical data buffered: "), dataKey, BLINKER_F("="), value);
            #else
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Historical data upload not supported in BLE mode"));
            #endif
        }
        
        void uploadHistorical(const char* dataKey, int16_t value)
        {
            // 历史数据上传仅支持WiFi模式（需要云端存储）
            #if defined(BLINKER_WIFI)
            Blinker.chartDataUpload(dataKey, value);
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Historical data buffered: "), dataKey, BLINKER_F("="), value);
            #else
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Historical data upload not supported in BLE mode"));
            #endif
        }
        
        void uploadHistorical(const char* dataKey, uint32_t value)
        {
            // 历史数据上传仅支持WiFi模式（需要云端存储）
            #if defined(BLINKER_WIFI)
            Blinker.chartDataUpload(dataKey, value);
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Historical data buffered: "), dataKey, BLINKER_F("="), value);
            #else
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Historical data upload not supported in BLE mode"));
            #endif
        }
        
        void uploadHistorical(const char* dataKey, int32_t value)
        {
            // 历史数据上传仅支持WiFi模式（需要云端存储）
            #if defined(BLINKER_WIFI)
            Blinker.chartDataUpload(dataKey, value);
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Historical data buffered: "), dataKey, BLINKER_F("="), value);
            #else
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Historical data upload not supported in BLE mode"));
            #endif
        }
        
        void uploadHistorical(const char* dataKey, float value)
        {
            // 历史数据上传仅支持WiFi模式（需要云端存储）
            #if defined(BLINKER_WIFI)
            Blinker.chartDataUpload(dataKey, value);
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Historical data buffered: "), dataKey, BLINKER_F("="), value);
            #else
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Historical data upload not supported in BLE mode"));
            #endif
        }
        
        void uploadHistorical(const char* dataKey, double value)
        {
            // 历史数据上传仅支持WiFi模式（需要云端存储）
            #if defined(BLINKER_WIFI)
            Blinker.chartDataUpload(dataKey, value);
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Historical data buffered: "), dataKey, BLINKER_F("="), value);
            #else
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Historical data upload not supported in BLE mode"));
            #endif
        }
        
        void uploadRealtime(const char* dataKey, uint8_t value)
        {
            Blinker.print(dataKey, value);
            Blinker.chartDataUpload(dataKey, value);
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Realtime data sent: "), dataKey, BLINKER_F("="), value);
        }
        
        void uploadRealtime(const char* dataKey, int8_t value)
        {
            Blinker.print(dataKey, value);
            Blinker.chartDataUpload(dataKey, value);
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Realtime data sent: "), dataKey, BLINKER_F("="), value);
        }
        
        void uploadRealtime(const char* dataKey, uint16_t value)
        {
            Blinker.print(dataKey, value);
            Blinker.chartDataUpload(dataKey, value);
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Realtime data sent: "), dataKey, BLINKER_F("="), value);
        }
        
        void uploadRealtime(const char* dataKey, int16_t value)
        {
            Blinker.print(dataKey, value);
            Blinker.chartDataUpload(dataKey, value);
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Realtime data sent: "), dataKey, BLINKER_F("="), value);
        }
        
        void uploadRealtime(const char* dataKey, uint32_t value)
        {
            Blinker.print(dataKey, value);
            Blinker.chartDataUpload(dataKey, value);
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Realtime data sent: "), dataKey, BLINKER_F("="), value);
        }
        
        void uploadRealtime(const char* dataKey, int32_t value)
        {
            Blinker.print(dataKey, value);
            Blinker.chartDataUpload(dataKey, value);
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Realtime data sent: "), dataKey, BLINKER_F("="), value);
        }
        
        void uploadRealtime(const char* dataKey, float value)
        {
            Blinker.print(dataKey, value);
            Blinker.chartDataUpload(dataKey, value);
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Realtime data sent: "), dataKey, BLINKER_F("="), value);
        }
        
        void uploadRealtime(const char* dataKey, double value)
        {
            Blinker.print(dataKey, value);
            Blinker.chartDataUpload(dataKey, value);
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Realtime data sent: "), dataKey, BLINKER_F("="), value);
        }
};

inline void BlinkerChartManager::registerChart(BlinkerChart* chart) {
    if (chartCount < BLINKER_MAX_WIDGET_SIZE) {
        charts[chartCount++] = chart;
        BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Auto-registered: "), chart->getName());
    }
}

inline void BlinkerChartManager::setupDataStorage(uint32_t interval_seconds) {
    #if defined(BLINKER_WIFI)
    if (!dataStorageSetup) {
        Blinker.attachDataStorage(globalDataCallback, interval_seconds);
        dataStorageSetup = true;
        BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Auto-setup data storage, interval: "), interval_seconds, BLINKER_F(" seconds"));
    }
    #endif
}

inline void BlinkerChartManager::globalDataCallback() {
    BlinkerChartManager& manager = getInstance();
    BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Global callback triggered, dispatching to "), manager.chartCount, BLINKER_F(" charts"));
    
    for (uint8_t i = 0; i < manager.chartCount; i++) {
        if (manager.charts[i]) {
            manager.charts[i]->triggerDataCallback();
        }
    }
}

inline void BlinkerChartManager::syncAllChartsMode() {
    uint32_t minInterval = UINT32_MAX;
    bool hasRealtimeChart = false;

    BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Syncing all chart modes"));

    for (uint8_t i = 0; i < chartCount; i++) {
        if (charts[i]) {
            charts[i]->forceSyncRealtimeMode();
            
            if (charts[i]->isRealtime()) {
                hasRealtimeChart = true;
                uint32_t intervalMs = charts[i]->getUploadInterval();
                if (intervalMs > 0) {
                    uint32_t intervalSec = intervalMs / 1000;
                    if (intervalSec < 5) intervalSec = 5; // 最小5秒间隔
                    if (intervalSec < minInterval) {
                        minInterval = intervalSec;
                    }
                }
            }
            
            BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Synced mode for: "), charts[i]->getName());
        }
    }
    
    #if defined(BLINKER_WIFI)
    if (hasRealtimeChart && minInterval != UINT32_MAX) {
        Blinker.updateDataStorageInterval(minInterval);
        BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] Updated global upload interval to: "), minInterval, BLINKER_F(" seconds"));
    } else if (!hasRealtimeChart) {
        Blinker.updateDataStorageInterval(60);  // 默认60秒
        BLINKER_LOG_ALL(BLINKER_F("[BlinkerChart] No realtime charts, restored default interval: 60 seconds"));
    }
    #endif
}

inline void BlinkerChartManager::triggerRealtimeCallback(const char* chartName) {
    for (uint8_t i = 0; i < chartCount; i++) {
        if (charts[i] && strcmp(charts[i]->getName(), chartName) == 0) {
            charts[i]->triggerRealtimeCallback();
            break;
        }
    }
}

#endif

// #endif
