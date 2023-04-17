

struct TPXHit
{
    double TOA;
    double TOT;
    int x;
    int y;
    TPXHit(double _toa, double _tot, int _x, int _y):
        TOA(_toa),
        TOT(_tot),
        x(_x),
        y(_y)
    {}
};

class TPXFrame
{
public:
    TPXFrame();

    std::vector<TPXHit> & hits() {return _hits;}

    size_t n_hits() {return _hits.size();}

private:
    std::vector<TPXHit> _hits;
    // TPXHit * _hits;
    double frame_start_time;
    // size_t _n_hits;
};

