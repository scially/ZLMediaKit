//
// Created by alex on 2021/4/6.
//

#include "TsPlayer.h"

namespace mediakit {

    TsPlayer::TsPlayer(const EventPoller::Ptr &poller):HttpTSPlayer(poller, true) {}

    TsPlayer::~TsPlayer() {}

    void TsPlayer::play(const string &strUrl) {
        _ts_url.append(strUrl);
        playTs();
    }

    void TsPlayer::teardown_l(const SockException &ex) {
        HttpClient::clear();
        shutdown(ex);
    }

    void TsPlayer::teardown() {
        teardown_l(SockException(Err_shutdown, "teardown"));
    }

    void TsPlayer::playTs() {
        if (waitResponse()) {
            //播放器目前还存活，正在下载中
            return;
        }
        WarnL << "fetch:" << _ts_url;
        _request_complete = false;
        weak_ptr<TsPlayer> weak_self = dynamic_pointer_cast<TsPlayer>(shared_from_this());
        setMethod("GET");
        sendRequest(_ts_url, 3600 * 2, 60);
    }

    void TsPlayer::onResponseCompleted() {
        //接收完毕
        teardown_l(SockException(Err_success, StrPrinter << _ts_url << ": play completed"));
    }

    void TsPlayer::onDisconnect(const SockException &ex) {
        WarnL <<  _ts_url << "   :" << ex.getErrCode() << " " << ex.what();
        if (_first) {
            //第一次失败，则播放失败
            _first = false;
            onPlayResult(ex);
            return;
        }
        if (ex.getErrCode() == Err_shutdown) {
            onShutdown(ex);
        }else{
            onResponseCompleted();
            onShutdown(ex);
        }
    }

    ssize_t TsPlayer::onResponseHeader(const string &status, const HttpClient::HttpHeader &header) {
        ssize_t ret = HttpTSPlayer::onResponseHeader(status, header);
        if (_first) {
            _first = false;
            onPlayResult(SockException(Err_success, "play success"));
        }
        return ret;
    }
}//namespace mediakit