/* Included only by the opt-in MP4 test build. The ordinary MKV/MP4 reader is
 * unchanged unless this particular engine was explicitly opened for the test.
 * Bounded compressed BL/RPU queues are paired by exact timestamps; four
 * HTTP slots are reserved before any range workers start. */
static void mp4_pair_reset(DtsEngine *e) {
  NvMp4Pair *m=e->mp4;
  if(!m)return;
  for(int i=0;i<MP4_BASE_QUEUE;i++)av_packet_unref(m->base[i]);
  for(int i=0;i<MP4_RPU_QUEUE;i++)m->rpu[i].size=0;
  m->base_count=m->rpu_count=m->aligned=m->align_drops=0;m->base_bytes=0;
}
static void mp4_pop_base(NvMp4Pair *m) {
  AVPacket *first=m->base[0];m->base_bytes-=(size_t)first->size;
  av_packet_unref(first);
  for(int i=1;i<m->base_count;i++)m->base[i-1]=m->base[i];
  m->base[--m->base_count]=first;
}
static int mp4_configure(DtsEngine *e) {
  if(!e->allow_mp4)return 0;
  if(!e->dv || !e->allow_p7 || !nv_dv7_available() ||
     !e->fmt->iformat || !strstr(e->fmt->iformat->name,"mov"))
    return fail(e,"MP4 test requires a dual-track P7 MP4",0);
  int base=-1,el=-1,videos=0;
  for(unsigned i=0;i<e->fmt->nb_streams;i++) {
    AVCodecParameters *p=e->fmt->streams[i]->codecpar;
    if(p->codec_type!=AVMEDIA_TYPE_VIDEO || (e->fmt->streams[i]->disposition&AV_DISPOSITION_ATTACHED_PIC))continue;
    videos++;
    if(p->codec_id!=AV_CODEC_ID_HEVC)return fail(e,"MP4 test requires HEVC BL and EL",0);
    const AVPacketSideData *d=av_packet_side_data_get(p->coded_side_data,p->nb_coded_side_data,AV_PKT_DATA_DOVI_CONF);
    if(d && d->size>=sizeof(AVDOVIDecoderConfigurationRecord)) {
      const AVDOVIDecoderConfigurationRecord *c=(const AVDOVIDecoderConfigurationRecord *)d->data;
      printf("[mp4-bd-test] track=%u tag=%08x profile=%d bl=%d el=%d rpu=%d\n",i,p->codec_tag,
             c->dv_profile,c->bl_present_flag,c->el_present_flag,c->rpu_present_flag);fflush(stdout);
      if(c->dv_profile!=7 || c->bl_present_flag || !c->el_present_flag || !c->rpu_present_flag || el>=0)
        return fail(e,"MP4 test refuses unrelated Dolby Vision tracks",0);
      el=(int)i;
    } else {
      if(base>=0)return fail(e,"MP4 test has ambiguous base tracks",0);
      base=(int)i;
    }
  }
  if(videos!=2 || base<0 || el<0)return fail(e,"MP4 test requires separate BL and P7 EL tracks",0);
  AVStream *b=e->fmt->streams[base],*v=e->fmt->streams[el];
  AVCodecParameters *bp=b->codecpar,*ep=v->codecpar;
  if(bp->width<=0 || bp->height<=0 || ep->width<=0 || ep->height<=0 ||
     bp->width<ep->width || bp->height<ep->height ||
     bp->color_trc!=AVCOL_TRC_SMPTE2084 || bp->color_primaries!=AVCOL_PRI_BT2020 ||
     b->time_base.num<=0 || b->time_base.den<=0 || v->time_base.num<=0 || v->time_base.den<=0 ||
     b->avg_frame_rate.num<=0 || av_cmp_q(b->avg_frame_rate,v->avg_frame_rate) ||
     bp->extradata_size<23 || ep->extradata_size<23 || bp->extradata[0]!=1 || ep->extradata[0]!=1)
    return fail(e,"MP4 BL/EL configuration is incompatible",0);
  e->mp4=av_mallocz(sizeof *e->mp4);
  if(!e->mp4)return fail(e,"MP4 pairing allocation",AVERROR(ENOMEM));
  for(int i=0;i<MP4_BASE_QUEUE;i++) {
    e->mp4->base[i]=av_packet_alloc();
    if(!e->mp4->base[i])return fail(e,"MP4 pairing packet allocation",AVERROR(ENOMEM));
  }
  e->mp4->base_length=(bp->extradata[21]&3)+1;
  e->mp4->el_length=(ep->extradata[21]&3)+1;
  e->mp4->el_stream=el;e->info.video_stream=base;
  mp4_pair_reset(e);
  printf("[mp4-bd-test] verified dual-track P7: BL=%d %dx%d EL=%d %dx%d; output=P8.1; reserved=16 MiB\n",
         base,bp->width,bp->height,el,ep->width,ep->height);fflush(stdout);
  return 0;
}
static int mp4_read_packet(DtsEngine *e) {
  NvMp4Pair *m=e->mp4;
  for(;;) {
    if(cancelled(e))return fail(e,"MP4 pairing cancelled",0);
    if(m->base_count) {
      AVPacket *base=m->base[0];
      AVStream *b=e->fmt->streams[e->info.video_stream],*v=e->fmt->streams[m->el_stream];
      if(base->pts==AV_NOPTS_VALUE || base->dts==AV_NOPTS_VALUE)
        return fail(e,"MP4 base timestamps absent",0);
      int match=-1,future=0;
      for(int i=0;i<MP4_RPU_QUEUE;i++)if(m->rpu[i].size) {
        if(m->rpu[i].pts==AV_NOPTS_VALUE || m->rpu[i].dts==AV_NOPTS_VALUE)
          return fail(e,"MP4 RPU timestamps absent",0);
        int order=av_compare_ts(base->dts,b->time_base,m->rpu[i].dts,v->time_base);
        if(!order) {
          if(av_compare_ts(base->pts,b->time_base,m->rpu[i].pts,v->time_base))
            return fail(e,"MP4 BL/EL presentation timestamps do not match",0);
          match=i;break;
        }
        if(order<0)future=1;
        else if(!m->aligned) {
          m->rpu[i].size=0;m->rpu_count--;m->align_drops++;
        }
      }
      if(m->align_drops>128)return fail(e,"MP4 seek alignment limit",0);
      if(match>=0) {
        size_t size=m->rpu[match].size;int length=m->base_length,old=base->size;
        if((length<4 && size>=(UINT32_C(1)<<(8*length))) ||
           (unsigned)old>NV_MP4_PACKET_MAX-size-(unsigned)length)
          return fail(e,"MP4 paired video packet exceeds its budget",0);
        int r=av_grow_packet(base,length+(int)size);
        if(r<0)return fail(e,"MP4 paired video allocation",r);
        for(int k=0;k<length;k++)base->data[old+k]=(uint8_t)(size>>(8*(length-k-1)));
        memcpy(base->data+old+length,m->rpu[match].data,size);
        m->rpu[match].size=0;m->rpu_count--;
        if(!m->aligned) {
          printf("[mp4-bd-test] BL/RPU aligned: dropped-preroll=%d pts-ns=%lld\n",m->align_drops,(long long)ns(base->pts,b->time_base));fflush(stdout);
        }
        m->aligned=1;
        /* The paired packet now belongs to the existing output/filter path. */
        m->base_bytes-=(size_t)old;
        av_packet_move_ref(e->in,base);
        mp4_pop_base(m);
        return 0;
      }
      if(future) {
        if(m->aligned)return fail(e,"MP4 RPU missing for a base picture",0);
        mp4_pop_base(m);m->align_drops++;continue;
      }
    }
    int r=av_read_frame(e->fmt,e->in);
    if(r<0) {
      if(r==AVERROR_EOF && (m->base_count || m->rpu_count))
        return fail(e,"MP4 ended with an unpaired BL/EL frame",0);
      return r;
    }
    if(av_packet_get_side_data(e->in,AV_PKT_DATA_ENCRYPTION_INFO,NULL)) {
      av_packet_unref(e->in);return fail(e,"Encrypted MP4 packet unsupported",0);
    }
    if(e->in->stream_index==e->info.video_stream) {
      if(m->base_count==MP4_BASE_QUEUE || e->in->size<=0 ||
         (unsigned)e->in->size>NV_MP4_PACKET_MAX-m->base_bytes)
        return fail(e,"MP4 BL interleave exceeds its bounded queue",0);
      m->base_bytes+=(size_t)e->in->size;
      av_packet_move_ref(m->base[m->base_count++],e->in);
    } else if(e->in->stream_index==m->el_stream) {
      int slot=0;while(slot<MP4_RPU_QUEUE && m->rpu[slot].size)slot++;
      if(slot==MP4_RPU_QUEUE)return fail(e,"MP4 RPU interleave exceeds its bounded queue",0);
      if(nv_mp4_rpu(e->in->data,e->in->size,m->el_length,m->rpu[slot].data,&m->rpu[slot].size)<0)
        return fail(e,"MP4 EL has missing, duplicate or invalid RPU",0);
      m->rpu[slot].pts=e->in->pts;m->rpu[slot].dts=e->in->dts;m->rpu_count++;
      av_packet_unref(e->in);
    } else return 0;
  }
}
