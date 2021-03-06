/**
* Copyright (C) Mellanox Technologies Ltd. 2001-2015.  ALL RIGHTS RESERVED.
*
* See file LICENSE for terms.
*/

#include "ucp_test.h"
#include "poll.h"

#include <algorithm>

class test_ucp_wakeup : public ucp_test {
public:
    static ucp_params_t get_ctx_params() {
        ucp_params_t params = ucp_test::get_ctx_params();
        params.features |= UCP_FEATURE_TAG | UCP_FEATURE_WAKEUP;
        return params;
    }

protected:
    static void send_completion(void *request, ucs_status_t status) {
        ++comp_cntr;
    }

    static void recv_completion(void *request, ucs_status_t status,
                                ucp_tag_recv_info_t *info) {
        ++comp_cntr;
    }

    void wait(void *req) {
        do {
            progress();
        } while (!ucp_request_is_completed(req));
        ucp_request_release(req);
    }

    static size_t comp_cntr;
};

size_t test_ucp_wakeup::comp_cntr = 0;

UCS_TEST_P(test_ucp_wakeup, efd)
{
    int recv_efd;
    struct pollfd polled;
    ucp_worker_h recv_worker;
    const ucp_datatype_t DATATYPE = ucp_dt_make_contig(1);
    const uint64_t TAG = 0xdeadbeef;
    uint64_t send_data = 0x12121212;
    void *req;
    ucs_status_t status;

    polled.events = POLLIN;
    sender().connect(&receiver());

    recv_worker = receiver().worker();
    ASSERT_UCS_OK(ucp_worker_get_efd(recv_worker, &recv_efd));

    polled.fd = recv_efd;

    do {
        status = ucp_worker_arm(recv_worker);
    } while (UCS_ERR_BUSY == status);
    ASSERT_EQ(UCS_OK, status);

    req = ucp_tag_send_nb(sender().ep(), &send_data, sizeof(send_data), DATATYPE,
                          TAG, send_completion);
    if (UCS_PTR_IS_PTR(req)) {
        wait(req);
    } else {
        ASSERT_UCS_OK(UCS_PTR_STATUS(req));
    }

    ASSERT_EQ(1, poll(&polled, 1, 1000*ucs::test_time_multiplier()));
    EXPECT_EQ(ucp_worker_arm(recv_worker), UCS_ERR_BUSY);

    uint64_t recv_data = 0;
    req = ucp_tag_recv_nb(receiver().worker(), &recv_data, sizeof(recv_data),
                          DATATYPE, TAG, (ucp_tag_t)-1, recv_completion);
    wait(req);

    close(recv_efd);
    ucp_worker_flush(sender().worker());
    EXPECT_EQ(send_data, recv_data);
}

UCS_TEST_P(test_ucp_wakeup, signal)
{
    int efd;
    ucp_worker_h worker;
    struct pollfd polled;

    polled.events = POLLIN;

    worker = sender().worker();
    ASSERT_UCS_OK(ucp_worker_get_efd(worker, &efd));

    polled.fd = efd;
    EXPECT_EQ(0, poll(&polled, 1, 0));
    ASSERT_UCS_OK(ucp_worker_arm(worker));
    ASSERT_UCS_OK(ucp_worker_signal(worker));
    EXPECT_EQ(1, poll(&polled, 1, 0));
    ASSERT_UCS_OK(ucp_worker_arm(worker));
    EXPECT_EQ(0, poll(&polled, 1, 0));

    ASSERT_UCS_OK(ucp_worker_signal(worker));
    ASSERT_UCS_OK(ucp_worker_signal(worker));
    EXPECT_EQ(1, poll(&polled, 1, 0));
    ASSERT_UCS_OK(ucp_worker_arm(worker));
    EXPECT_EQ(0, poll(&polled, 1, 0));

    close(efd);
}

UCP_INSTANTIATE_TEST_CASE(test_ucp_wakeup)

class test_ucp_wakeup_events : public test_ucp_wakeup
{
public:
    static std::vector<ucp_test_param>
    enum_test_params(const ucp_params_t& ctx_params,
                     const ucp_worker_params_t& worker_params,
                     const std::string& name,
                     const std::string& test_case_name,
                     const std::string& tls);

    void do_tx_rx_events_test(const std::vector<std::string>& transports,
                              unsigned events);

private:
    static void setup_worker_param_events(ucp_worker_params_t& params,
                                          unsigned events);
};

std::vector<ucp_test_param>
test_ucp_wakeup_events::enum_test_params(const ucp_params_t& ctx_params,
                                         const ucp_worker_params_t& worker_params,
                                         const std::string& name,
                                         const std::string& test_case_name,
                                         const std::string& tls)
{
    std::vector<ucp_test_param> result;
    ucp_worker_params_t worker_params_tmp = worker_params;

    /* TODO: add RMA and AMO after required optimizations */
    setup_worker_param_events(worker_params_tmp, UCP_WAKEUP_TAG_SEND);
    generate_test_params_variant(ctx_params, worker_params_tmp, name,
                                 test_case_name + "/tag_send", tls, 0, result);

    setup_worker_param_events(worker_params_tmp, UCP_WAKEUP_TAG_RECV);
    generate_test_params_variant(ctx_params, worker_params_tmp, name,
                                 test_case_name + "/tag_recv", tls, 0, result);

    setup_worker_param_events(worker_params_tmp,
                              UCP_WAKEUP_TAG_SEND | UCP_WAKEUP_TAG_RECV);
    generate_test_params_variant(ctx_params, worker_params_tmp, name,
                                 test_case_name + "/all", tls, 0, result);
    return result;
}

static inline bool is_any_tl_inuse(const std::vector<std::string>& transports,
                                   const std::vector<std::string>& tl_names)
{
    std::vector<std::string>::const_iterator i = tl_names.begin();
    for (; i != tl_names.end(); ++i) {
        if (transports.end() !=
            std::find_if(transports.begin(),
                         transports.end(),
                         std::bind2nd(std::equal_to<std::string>(), *i))) {
            return true;
        }
    }

    return false;
}

void
test_ucp_wakeup_events::do_tx_rx_events_test(const std::vector<std::string>& transports,
                                             unsigned events)
{
    int efd[2];
    struct pollfd polled[2];
    ucp_test_base::entity *p_entity[2] = { &sender(), &receiver() };
    const ucp_datatype_t DATATYPE = ucp_dt_make_contig(1);
    const uint64_t TAG = 0xdeadbeef;
    uint64_t send_data = 0x12121212;
    const size_t msg_count = 1001;
    std::vector<void *> req(2*msg_count, NULL);

    ucs_status_t status;

    /* UD based transports may cause extra events */
    const char*  ud_tls[]   = { "\\ud", "\\ud_mlx5" };
    const size_t ud_tls_cnt = (sizeof(ud_tls) / sizeof(ud_tls[0]));
    const bool has_ud = is_any_tl_inuse(transports,
                                        std::vector<std::string>(ud_tls,
                                                                 ud_tls +
                                                                 ud_tls_cnt));

    polled[0].events = polled[1].events = POLLIN;

    p_entity[0]->connect(p_entity[1]);
    p_entity[1]->connect(p_entity[0]);

    ASSERT_UCS_OK(ucp_worker_get_efd(p_entity[0]->worker(), &efd[0]));
    ASSERT_UCS_OK(ucp_worker_get_efd(p_entity[1]->worker(), &efd[1]));

    polled[0].fd = efd[0];
    polled[1].fd = efd[1];

    do {
        status = ucp_worker_arm(p_entity[0]->worker());
    } while (UCS_ERR_BUSY == status);
    ASSERT_EQ(UCS_OK, status);

    do {
        status = ucp_worker_arm(p_entity[1]->worker());
    } while (UCS_ERR_BUSY == status);
    ASSERT_EQ(UCS_OK, status);

    uint64_t recv_data = 0;
    size_t req_cntr = 0;
    comp_cntr = 0;

    /* Do not care about data in this test, just count events */
    for (size_t i = 0; i < msg_count; ++i) {
        req[i] = ucp_tag_send_nb(p_entity[0]->ep(), &send_data, sizeof(send_data),
                                 DATATYPE, TAG, send_completion);
        req_cntr += UCS_PTR_IS_PTR(req[i]) ? 1 : 0;
    }

    for (size_t i = msg_count; i < 2*msg_count; ++i) {
        req[i] = ucp_tag_recv_nb(p_entity[1]->worker(), &recv_data, sizeof(recv_data),
                                 DATATYPE, TAG, (ucp_tag_t)-1, recv_completion);
        req_cntr += UCS_PTR_IS_PTR(req[i]) ? 1 : 0;
    }

    /* wait until all messages are completed */
    for (size_t i = 0; i < 2*msg_count; ++i) {
        if (UCS_PTR_IS_PTR(req[i])) {
            wait(req[i]);
        } else {
            ASSERT_UCS_OK(UCS_PTR_STATUS(req[i]));
        }
    }

    /*
     * NOTE: ud and udx tests are failing under valgrind w/o
     *       counting of completions
     */
    EXPECT_EQ(req_cntr, comp_cntr);

    int nfd_exp = 0;
    if (events & (UCP_WAKEUP_TAG_SEND |
                  UCP_WAKEUP_TAG_RECV /* FIXME: PR #1277 (RNDV) */)) {
        ++nfd_exp;
    }
    if (events & (UCP_WAKEUP_TAG_RECV)) {
        ++nfd_exp;
    }

    if (events & UCP_WAKEUP_TAG_RECV) {
        /* UCP_WAKEUP_RX_AM may cause events on sender side because of internal
         * messages arriving, these are not TX-events */
        EXPECT_LE(nfd_exp, poll(polled, 2, 1000*ucs::test_time_multiplier()));
    } else {
        if (has_ud) {
            EXPECT_LE(nfd_exp, poll(polled, 2, 1000*ucs::test_time_multiplier()));
        } else {
            EXPECT_EQ(nfd_exp, poll(polled, 2, 1000*ucs::test_time_multiplier()));
        }
    }

    if (events & (UCP_WAKEUP_TAG_SEND |
                  UCP_WAKEUP_TAG_RECV /* FIXME: PR #1277 (RNDV) */)) {
        EXPECT_TRUE(polled[0].revents);
    } else {
        /* UCP_WAKEUP_RX_AM may cause events on sender side because of internal
         * messages arriving, these are not TX-events */
    }

    if (events & UCP_WAKEUP_TAG_RECV) {
        EXPECT_TRUE(polled[1].revents);
    } else {
        if (!has_ud) {
            EXPECT_FALSE(polled[1].revents);
        }
    }

    close(efd[0]);
    close(efd[1]);
    ucp_worker_flush(p_entity[0]->worker());
    ucp_worker_flush(p_entity[1]->worker());
}

void
test_ucp_wakeup_events::setup_worker_param_events(ucp_worker_params_t& params,
                                                  unsigned events)
{
    params.field_mask |= UCP_WORKER_PARAM_FIELD_EVENTS;
    params.events = events;
}

UCS_TEST_P(test_ucp_wakeup_events, events)
{
    UCS_TEST_SKIP_R("Functionality is not implemented yet");

    EXPECT_TRUE(GetParam().worker_params.field_mask &
                UCP_WORKER_PARAM_FIELD_EVENTS);
    do_tx_rx_events_test(GetParam().transports,
                         GetParam().worker_params.events);
}

UCP_INSTANTIATE_TEST_CASE(test_ucp_wakeup_events)
