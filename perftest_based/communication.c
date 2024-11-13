
/******************************************************************************
 *
 ******************************************************************************/
int create_comm_struct(struct perftest_comm *comm,
        struct perftest_parameters *user_param)
{
    MAIN_ALLOC(comm->rdma_params, struct perftest_parameters, 1, return_error);
    memset(comm->rdma_params, 0, sizeof(struct perftest_parameters));

    comm->rdma_params->port		   	= user_param->port;
    comm->rdma_params->ai_family	   	= user_param->ai_family;
    comm->rdma_params->sockfd      		= -1;
    comm->rdma_params->gid_index   		= user_param->gid_index;
    comm->rdma_params->gid_index2 		= user_param->gid_index2;
    comm->rdma_params->use_rdma_cm 		= user_param->use_rdma_cm;
    comm->rdma_params->servername  		= user_param->servername;
    comm->rdma_params->machine 	   	= user_param->machine;
    comm->rdma_params->side		   	= LOCAL;
    comm->rdma_params->verb		   	= user_param->verb;
    comm->rdma_params->use_mcg	   	= user_param->use_mcg;
    comm->rdma_params->duplex	   	= user_param->duplex;
    comm->rdma_params->tos         		= DEF_TOS;
    comm->rdma_params->use_xrc	   	= user_param->use_xrc;
    comm->rdma_params->connection_type	= user_param->connection_type;
    comm->rdma_params->output      		= user_param->output;
    comm->rdma_params->report_per_port 	= user_param->report_per_port;
    comm->rdma_params->retry_count		= user_param->retry_count;
    comm->rdma_params->qp_timeout		= user_param->qp_timeout;
    comm->rdma_params->mr_per_qp		= user_param->mr_per_qp;
    comm->rdma_params->dlid			= user_param->dlid;
    comm->rdma_params->cycle_buffer         = user_param->cycle_buffer;
    comm->rdma_params->use_old_post_send	= user_param->use_old_post_send;
    comm->rdma_params->source_ip		= user_param->source_ip;
    comm->rdma_params->has_source_ip	= user_param->has_source_ip;
    comm->rdma_params->memory_type		= MEMORY_HOST;
    comm->rdma_params->memory_create	= host_memory_create;

    if (user_param->use_rdma_cm) {

        MAIN_ALLOC(comm->rdma_ctx, struct pingpong_context, 1, free_rdma_params);
        memset(comm->rdma_ctx, 0, sizeof(struct pingpong_context));

        comm->rdma_params->tx_depth = 1;
        comm->rdma_params->rx_depth = 1;
        comm->rdma_params->connection_type = RC;
        comm->rdma_params->num_of_qps = 1;
        comm->rdma_params->verb	= SEND;
        comm->rdma_params->size = sizeof(struct pingpong_dest);
        comm->rdma_ctx->context = NULL;

        comm->rdma_ctx->memory = comm->rdma_params->memory_create(comm->rdma_params);
        if (comm->rdma_ctx->memory == NULL)
            goto free_rdma_ctx;

        MAIN_ALLOC(comm->rdma_ctx->mr, struct ibv_mr*, user_param->num_of_qps, free_memory_ctx);
        MAIN_ALLOC(comm->rdma_ctx->buf, void* , user_param->num_of_qps, free_mr);
        MAIN_ALLOC(comm->rdma_ctx->qp,struct ibv_qp*,comm->rdma_params->num_of_qps, free_buf);
        #ifdef HAVE_IBV_WR_API
        MAIN_ALLOC(comm->rdma_ctx->qpx,struct ibv_qp_ex*,comm->rdma_params->num_of_qps, free_qp);
        #endif
        #ifdef HAVE_DCS
        MAIN_ALLOC(comm->rdma_ctx->dci_stream_id,uint32_t, comm->rdma_params->num_of_qps, free_qpx);
        #endif
        comm->rdma_ctx->buff_size = user_param->cycle_buffer;

        if (create_rdma_resources(comm->rdma_ctx,comm->rdma_params)) {
            fprintf(stderr," Unable to create the resources needed by comm struct\n");
            goto free_mem;
        }
    }

    if ((user_param->counter_ctx) && (counters_open(user_param->counter_ctx,
        user_param->ib_devname, user_param->ib_port))) {
        fprintf(stderr," Unable to access performance counters\n");
        if (user_param->use_rdma_cm)
            goto free_mem;
        else
            goto free_rdma_params;
    }

    return SUCCESS;

free_mem: __attribute__((unused))
    #ifdef HAVE_DCS
    free(comm->rdma_ctx->dci_stream_id);
    #endif
// cppcheck-suppress unusedLabelConfiguration
free_qpx: __attribute__((unused))
    #ifdef HAVE_IBV_WR_API
    free(comm->rdma_ctx->qpx);
    #endif
// cppcheck-suppress unusedLabelConfiguration
free_qp:
    free(comm->rdma_ctx->qp);
free_buf:
    free(comm->rdma_ctx->buf);
free_mr:
    free(comm->rdma_ctx->mr);
free_memory_ctx:
    comm->rdma_ctx->memory->destroy(comm->rdma_ctx->memory);
free_rdma_ctx:
    free(comm->rdma_ctx);
free_rdma_params:
    free(comm->rdma_params);
return_error:
    return FAILURE;
}

int hand_shake(struct) {
    
}