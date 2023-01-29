#include "mtcnn_detect.h"
#include <iostream>

static cv::dnn::Net pnet;
static cv::dnn::Net rnet;
static cv::dnn::Net onet;
static const std::vector<cv::String> nets_outblob_names{"prob", "bbox_pred"};
static std::vector<float> working_scales;
static const float pnet_winsize = 12.0f;
static unsigned int num_working_scale;
static const int rnet_winsize = 24;


static void nms_bounding_box
    (
    std::vector<obj_info>& inboxes,
    float thresh,
    char method_type,
    std::vector<obj_info>& outboxes
    )
{
    if (inboxes.size() == 0)
        {
        return;
        }

    std::sort(inboxes.begin(), inboxes.end(), [](const obj_info &a, const obj_info &b){ return a.bbox.score > b.bbox.score;});

    int select_idx = 0;
    int num_bbox = inboxes.size();
    std::vector<unsigned char> mask_merged(num_bbox, 0);
    unsigned char all_merged = 0;

    while (!all_merged)
        {
        while (select_idx < num_bbox && mask_merged[select_idx] == 1)
            {
            ++select_idx;
            }
        if (select_idx == num_bbox)
            {
            all_merged = 1;
            continue;
            }

        outboxes.push_back(inboxes[select_idx]);
        mask_merged[select_idx] = 1;

        obj_box select_bbox = inboxes[select_idx].bbox;
        float area1 = (select_bbox.r - select_bbox.l + 1) * (select_bbox.b - select_bbox.t + 1);
        float x1 = select_bbox.l;
        float y1 = select_bbox.t;
        float x2 = select_bbox.r;
        float y2 = select_bbox.b;

        ++select_idx;
//#pragma omp parallel for num_threads(threads_num)
        for (int i = select_idx; i < num_bbox; ++i)
            {
            if (mask_merged[i] == 1)
                {
                continue;
                }
            obj_box & bbox_i = inboxes[i].bbox;
            float x = std::max<float>(x1, bbox_i.l);
            float y = std::max<float>(y1, bbox_i.t);
            float w = std::min<float>(x2, bbox_i.r) - x + 1;
            float h = std::min<float>(y2, bbox_i.b) - y + 1;
            if (w <= 0 || h <= 0)
                {
                continue;
                }
            float area2 = (bbox_i.r - bbox_i.l + 1) * (bbox_i.b - bbox_i.t + 1);
            float area_intersect = w * h;

            switch (method_type)
                {
                case 'u':
                    if (area_intersect / (area1 + area2 - area_intersect) > thresh)
                        {
                        mask_merged[i] = 1;
                        }
                    break;
                case 'm':
                    if (area_intersect / std::min<float>(area1, area2) > thresh)
                        {
                        mask_merged[i] = 1;
                        }
                    break;
                default:
                    break;
                }
            }
        }
}

static void regress_boxes
    (
    std::vector<obj_info>& bboxes
    )
{
    unsigned int num_bboxes = bboxes.size();
//#pragma omp parallel for num_threads(threads_num)
    for (unsigned int i = 0; i < num_bboxes; ++i)
        {
        obj_box &bbox = bboxes[i].bbox;
        float *bbox_reg = bboxes[i].bbox_reg;
        float w = bbox.r - bbox.l;
        float h = bbox.b - bbox.t;
        bbox.l += bbox_reg[0] * w;
        bbox.t += bbox_reg[1] * h;
        bbox.r += bbox_reg[2] * w;
        bbox.b += bbox_reg[3] * h;
        }
}

static void make_square
    (
    std::vector<obj_info>& bboxes
    )
{
    unsigned int num_bboxes = bboxes.size();
//#pragma omp parallel for num_threads(threads_num)
    for (unsigned int i = 0; i < num_bboxes; ++i)
        {
        obj_box &bbox = bboxes[i].bbox;
        float w = bbox.r - bbox.l;
        float h = bbox.b - bbox.t;
        float xcenter = (bbox.r + bbox.l) * 0.5f;
        float ycenter = (bbox.b + bbox.t) * 0.5f;
        float side = h > w ? h : w;
        side *= 0.5f;
        bbox.l = xcenter - side;
        bbox.t = ycenter - side;
        bbox.r = xcenter + side;
        bbox.b = ycenter + side;
        }
}

void init_mtcnn(const int srcw, const int srch, cv::String model_path_root)
{
    // head with onet parameter reduction
    pnet = cv::dnn::readNetFromCaffe(model_path_root + "/proto/p.prototxt", model_path_root + "/model/pnet_iter_446000.caffemodel");
    rnet = cv::dnn::readNetFromCaffe(model_path_root + "/proto/r.prototxt", model_path_root + "/model/rnet_iter_116000.caffemodel");
    onet = cv::dnn::readNetFromCaffe(model_path_root + "/proto/o.prototxt", model_path_root + "/model/onet_iter_90000.caffemodel");

    float scale_base = pnet_winsize / min_objsize;
    float current_netim_side_size = std::min(srcw, srch);
    const float min_netim_side_size = pnet_winsize * current_netim_side_size / max_objsize;
    current_netim_side_size *= scale_base;

    while (current_netim_side_size > min_netim_side_size)
        {
        working_scales.push_back(scale_base);
        scale_base *= pyramid_factor;
        current_netim_side_size *= pyramid_factor;
        }
    num_working_scale = working_scales.size();

    std::cout << "scales: " << std::endl;
    for(unsigned int i = 0; i < num_working_scale; ++i)
        {
        std::cout << working_scales[i] << std::endl;
        }
    std::cout << "------------" << std::endl;
}

void run_mtcnn
    (
    cv::Mat& im,
    std::vector<obj_info>& onet_boxes
    )
{
    const int IMW = im.cols;
    const int IMH = im.rows;
    std::vector<obj_info> pnet_boxes_in_all_scales;
    for (unsigned int i = 0; i < num_working_scale; ++i)
        {
        const float scale = working_scales[i];
        const float scale_inv = 1.0f / scale;
        const float candidate_winsize = pnet_winsize * scale_inv;
        int netw = std::ceil(IMW * working_scales[i]);
        int neth = std::ceil(IMH * working_scales[i]);
        cv::Mat netim;
        cv::resize(im, netim, cv::Size(netw, neth));
        cv::Mat inblob = cv::dnn::blobFromImage(netim, 0.0078125f, cv::Size(), cv::Scalar(128, 128, 128), false);
        pnet.setInput(inblob, "data");
        std::vector<cv::Mat> outblobs;
        pnet.forward(outblobs, nets_outblob_names);

        cv::Mat clsprob = outblobs[0];
        cv::Mat boxroi = outblobs[1];
        const int netoutw = clsprob.size[3];
        const int netouth = clsprob.size[2];
        const int netoutsize = netoutw * netouth;
        const float *scores_data = (float *)(clsprob.data);
        const float *reg_data = (float *)(boxroi.data);
        // get positive obj prob
        scores_data += netoutsize;
        // generate bounding box
        std::vector<obj_info> candidate_boxes;
        int idx = 0;
        for (int y = 0; y < netouth; ++y)
            {
            for (int x = 0; x < netoutw; ++x)
                {
                if (scores_data[idx] > pnet_th)
                    {
                    obj_info instance_info;
                    obj_box &instance_box = instance_info.bbox;
                    instance_box.l = (x << 1) * scale_inv;
                    instance_box.t = (y << 1) * scale_inv;
                    instance_box.r = instance_box.l + candidate_winsize;
                    instance_box.b = instance_box.t + candidate_winsize;
                    instance_box.score = scores_data[idx];
                    instance_info.bbox_reg[0] = reg_data[idx];
                    instance_info.bbox_reg[1] = reg_data[idx + netoutsize];
                    instance_info.bbox_reg[2] = reg_data[idx + netoutsize + netoutsize];
                    instance_info.bbox_reg[3] = reg_data[idx + netoutsize + netoutsize + netoutsize];
                    candidate_boxes.push_back(instance_info);
                    }
                ++idx;
                }
            }
        std::vector<obj_info> nms_boxes;
        nms_bounding_box(candidate_boxes, 0.5f, 'u', nms_boxes);
        if (nms_boxes.size() > 0)
            {
            pnet_boxes_in_all_scales.insert(pnet_boxes_in_all_scales.end(), nms_boxes.begin(), nms_boxes.end());
            }
        }
#ifdef SHOW_PNET_RESULT
    if (pnet_boxes_in_all_scales.size() != 0)
        {
        nms_bounding_box(pnet_boxes_in_all_scales, pnet_merge_th, 'u', onet_boxes);
        regress_boxes(onet_boxes);
        make_square(onet_boxes);
        }
#else
    std::vector<obj_info> pnet_boxes;
    if (pnet_boxes_in_all_scales.size() != 0)
        {
        nms_bounding_box(pnet_boxes_in_all_scales, pnet_merge_th, 'u', pnet_boxes);
        regress_boxes(pnet_boxes);
        make_square(pnet_boxes);
        }
    unsigned int num_pnet_boxes = pnet_boxes.size();
    if (num_pnet_boxes == 0)
        {
        return;
        }


    // rnet
    if (num_pnet_boxes > max_pnet_bbox_num)
        {
        num_pnet_boxes = max_pnet_bbox_num;
        }
    //std::cout << "p: " << num_pnet_boxes << std::endl;
    std::vector<cv::Mat> rnet_inputs;
    for (unsigned int n = 0; n < num_pnet_boxes; ++n)
        {
        obj_box &box = pnet_boxes[n].bbox;
        const int x1 = (int)(box.l);
        const int y1 = (int)(box.t);
        const int x2 = (int)(box.r);
        const int y2 = (int)(box.b);
        const int h = y2 - y1;
        const int w = x2 - x1;
        cv::Mat roi = cv::Mat::zeros(h, w, CV_8UC3);
        if (x1 < 0 || y1 < 0 || x2 > IMW || y2 > IMH)
            {
            int vx1 = x1;
            int sx = 0;
            if (x1 < 0)
                {
                vx1 = 0;
                sx = -x1;
                }
            int vy1 = y1;
            int sy = 0;
            if (y1 < 0)
                {
                vy1 = 0;
                sy = -y1;
                }
            int vx2 = x2;
            if (x2 > IMW)
                {
                vx2 = IMW;
                }
            int vy2 = y2;
            if (y2 > IMH)
                {
                vy2 = IMH;
                }
            const int vw = vx2 - vx1;
            const int vh = vy2 - vy1;
            im(cv::Range(vy1, vy2), cv::Range(vx1, vx2)).copyTo(roi(cv::Range(sy, sy + vh), cv::Range(sx, sx + vw)));
            }
        else
            {
            im(cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2))).copyTo(roi);
            }

        cv::resize(roi, roi, cv::Size(rnet_winsize, rnet_winsize));
        rnet_inputs.push_back(roi);
        }

    cv::Mat inblob = cv::dnn::blobFromImages(rnet_inputs, 0.0078125f, cv::Size(), cv::Scalar(128, 128, 128), false);
    rnet.setInput(inblob, "data");
    std::vector<cv::Mat> rnet_outblobs;
    rnet.forward(rnet_outblobs, nets_outblob_names);

    cv::Mat clsprob = rnet_outblobs[0];
    cv::Mat boxroi = rnet_outblobs[1];
    const float *scores_data = (float *)(clsprob.data);
    const float *reg_data = (float *)(boxroi.data);

    std::vector<obj_info> rnet_candidate_boxes;
    for (unsigned int k = 0; k < num_pnet_boxes; ++k)
        {
        const float score = scores_data[2 * k + 1];
        if (score > rnet_th)
            {
            obj_info instance_info;
            instance_info.bbox = pnet_boxes[k].bbox;
            instance_info.bbox.score = score;
            instance_info.bbox_reg[0] =  reg_data[4 * k];
            instance_info.bbox_reg[1] =  reg_data[4 * k + 1];
            instance_info.bbox_reg[2] =  reg_data[4 * k + 2];
            instance_info.bbox_reg[3] =  reg_data[4 * k + 3];
            rnet_candidate_boxes.push_back(instance_info);
            }
        }
#ifdef SHOW_RNET_RESULT
    nms_bounding_box(rnet_candidate_boxes, rnet_merge_th, 'u', onet_boxes);
    regress_boxes(onet_boxes);
    make_square(onet_boxes);
#else
    std::vector<obj_info> rnet_boxes;
    nms_bounding_box(rnet_candidate_boxes, rnet_merge_th, 'u', rnet_boxes);
    regress_boxes(rnet_boxes);
    make_square(rnet_boxes);
    unsigned int num_rnet_boxes = rnet_boxes.size();
    if (num_rnet_boxes == 0)
        {
        return;
        }


    // onet
    if (num_rnet_boxes > max_rnet_bbox_num)
        {
        num_rnet_boxes = max_rnet_bbox_num;
        }
    //std::cout << "r: " << num_rnet_boxes << std::endl;
    const int onet_winsize = 48;
    std::vector<cv::Mat> onet_inputs;
    for (unsigned int n = 0; n < num_rnet_boxes; ++n)
        {
        obj_box &box = rnet_boxes[n].bbox;
        //std::cout << box.l << ", " << box.t << ", " << box.r << ", " << box.b << std::endl;
        const int x1 = (int)(box.l);
        const int y1 = (int)(box.t);
        const int x2 = (int)(box.r);
        const int y2 = (int)(box.b);
        const int h = y2 - y1;
        const int w = x2 - x1;
        cv::Mat roi = cv::Mat::zeros(h, w, CV_8UC3);
        if (x1 < 0 || y1 < 0 || x2 > IMW || y2 > IMH)
            {
            int vx1 = x1;
            int sx = 0;
            if (x1 < 0)
                {
                vx1 = 0;
                sx = -x1;
                }
            int vy1 = y1;
            int sy = 0;
            if (y1 < 0)
                {
                vy1 = 0;
                sy = -y1;
                }
            int vx2 = x2;
            if (x2 > IMW)
                {
                vx2 = IMW;
                }
            int vy2 = y2;
            if (y2 > IMH)
                {
                vy2 = IMH;
                }
            const int vw = vx2 - vx1;
            const int vh = vy2 - vy1;
            im(cv::Range(vy1, vy2), cv::Range(vx1, vx2)).copyTo(roi(cv::Range(sy, sy + vh), cv::Range(sx, sx + vw)));
            }
        else
            {
            im(cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2))).copyTo(roi);
            }
        cv::resize(roi, roi, cv::Size(onet_winsize, onet_winsize));
        onet_inputs.push_back(roi);
        }

    inblob = cv::dnn::blobFromImages(onet_inputs, 0.0078125f, cv::Size(), cv::Scalar(128, 128, 128), false);
    onet.setInput(inblob, "data");
    std::vector<cv::Mat> onet_outblobs;
    onet.forward(onet_outblobs, nets_outblob_names);
    clsprob = onet_outblobs[0];
    boxroi = onet_outblobs[1];
    scores_data = (float *)(clsprob.data);
    reg_data = (float *)(boxroi.data);
    std::vector<obj_info> onet_candidate_boxes;
    for (unsigned int k = 0; k < num_rnet_boxes; ++k)
        {
        const float score = scores_data[2 * k + 1];
        if (score > onet_th)
            {
            obj_info instance_info;
            instance_info.bbox = rnet_boxes[k].bbox;
            instance_info.bbox.score = score;
            instance_info.bbox_reg[0] =  reg_data[4 * k];
            instance_info.bbox_reg[1] =  reg_data[4 * k + 1];
            instance_info.bbox_reg[2] =  reg_data[4 * k + 2];
            instance_info.bbox_reg[3] =  reg_data[4 * k + 3];
            onet_candidate_boxes.push_back(instance_info);
            }
        }

    regress_boxes(onet_candidate_boxes);
    nms_bounding_box(onet_candidate_boxes, 0.5f, 'm', onet_boxes);
#endif
#endif
}
