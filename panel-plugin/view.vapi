[CCode (cheader_filename = "view.h", cprefix = "Places", lower_case_cprefix = "places_")]
namespace Places {

    [Compact]
    [CCode (free_function = "places_view_finalize")]
    public class View {
        [CCode (cname = "places_view_init")]
        public View (Xfce.PanelPlugin plugin);
        public void finalize ();
    }
}

/* vim: set et ts=4 sw=4: */
