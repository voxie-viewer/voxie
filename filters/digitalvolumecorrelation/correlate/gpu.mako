<%def name='sum0(kernel_declaration, k_roi_batch)'>
${kernel_declaration}
{
    VIRTUAL_SKIP_THREADS;

    const VSIZE_T idz = virtual_global_id(0);
    const VSIZE_T idy = virtual_global_id(1);
    const VSIZE_T idbatch = virtual_global_id(2);

    float sum = 0;
    float prev = 0;

    for (int i = 0; i < ${filter_size}; i++)
    {
        sum += ${k_roi_batch.load_idx}(idbatch, i, idy, idz);
    }

    prev = ${k_roi_batch.load_idx}(idbatch, 0, idy, idz);
    ${k_roi_batch.store_idx}(idbatch, 0, idy, idz, sum);

    for (VSIZE_T idx = 1; idx < ${x_end}; idx++)
    {   
        sum += ${k_roi_batch.load_idx}(idbatch, idx + ${filter_size - 1}, idy, idz);
        sum -= prev;
        prev = ${k_roi_batch.load_idx}(idbatch, idx, idy, idz);

        ${k_roi_batch.store_idx}(idbatch, idx, idy, idz, sum);
    }

}
</%def>
<%def name='sum1(kernel_declaration, k_roi_batch)'>
${kernel_declaration}
{
    VIRTUAL_SKIP_THREADS;

    const VSIZE_T idz = virtual_global_id(0);
    const VSIZE_T idx = virtual_global_id(1);
    const VSIZE_T idbatch = virtual_global_id(2);

    float sum = 0;
    float prev = 0;

    for (int i = 0; i < ${filter_size}; i++)
    {
        sum += ${k_roi_batch.load_idx}(idbatch, idx, i, idz);
    }

    prev = ${k_roi_batch.load_idx}(idbatch, idx, 0, idz);
    ${k_roi_batch.store_idx}(idbatch, idx, 0, idz, sum);

    for (VSIZE_T idy = 1; idy < ${y_end}; idy++)
    {
        sum += ${k_roi_batch.load_idx}(idbatch, idx, idy + ${filter_size - 1}, idz);
        sum -= prev;
        prev = ${k_roi_batch.load_idx}(idbatch, idx, idy, idz);

        ${k_roi_batch.store_idx}(idbatch, idx, idy, idz, sum);
    }

}
</%def>
<%def name='sum2(kernel_declaration, k_roi_batch)'>
${kernel_declaration}
{
    VIRTUAL_SKIP_THREADS;

    const VSIZE_T idy = virtual_global_id(0);
    const VSIZE_T idx = virtual_global_id(1);
    const VSIZE_T idbatch = virtual_global_id(2);

    float sum = 0;
    float prev = 0;

    for (int i =  0; i < ${filter_size}; i++)
    {
        sum += ${k_roi_batch.load_idx}(idbatch, idx, idy, i);
    }

    prev = ${k_roi_batch.load_idx}(idbatch, idx, idy, 0);
    ${k_roi_batch.store_idx}(idbatch, idx, idy, 0, sum);

    for (VSIZE_T idz = 1; idz < ${z_end}; idz++)
    {
        sum += ${k_roi_batch.load_idx}(idbatch, idx, idy, idz + ${filter_size - 1});
        sum -= prev;
        prev = ${k_roi_batch.load_idx}(idbatch, idx, idy, idz);


        ${k_roi_batch.store_idx}(idbatch, idx, idy, idz, sum);
    }

}
</%def>
