<%def name='autocorr(kernel_declaration, k_output, k_image, k_indices)'>
${kernel_declaration}
{
    VIRTUAL_SKIP_THREADS;

    const VSIZE_T idx = virtual_global_id(0);
    const VSIZE_T idy = virtual_global_id(1);

    float sum = 0;

    uint x_start = ${k_indices.load_idx}(idx, 0);
    uint y_start = ${k_indices.load_idx}(idx, 1);
    uint z_start = ${k_indices.load_idx}(idx, 2);

    uint x_off = ${k_indices.load_idx}(idy, 0);
    uint y_off = ${k_indices.load_idx}(idy, 1);
    uint z_off = ${k_indices.load_idx}(idy, 2);

    for (int i=0; i<${x_end}; i++)
    {
        for (int j=0; j<${y_end}; j++)
        {
            for (int k=0; k<${z_end}; k++)
            {
                sum += ${k_image.load_idx}(i+x_start, j+y_start, k+z_start)
                         * ${k_image.load_idx}(i+x_off, j+y_off, k+z_off);
            }
        }
    }

    ${k_output.store_idx}(idx, idy, sum);
}
</%def>
