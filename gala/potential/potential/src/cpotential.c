#include <stdlib.h>
#include <math.h>
#include "cpotential.h"


void apply_rotate(double *q_in, double *R, int n_dim, int transpose,
                  double *q_out) {
    // NOTE: elsewhere, we enforce that rotation matrix only works for
    // ndim=2 or ndim=3, so here we can assume that!
    if (n_dim == 3) {
        if (transpose == 0) {
            q_out[0] = q_out[0] + R[0] * q_in[0] + R[1] * q_in[1] + R[2] * q_in[2];
            q_out[1] = q_out[1] + R[3] * q_in[0] + R[4] * q_in[1] + R[5] * q_in[2];
            q_out[2] = q_out[2] + R[6] * q_in[0] + R[7] * q_in[1] + R[8] * q_in[2];
        } else {
            q_out[0] = q_out[0] + R[0] * q_in[0] + R[3] * q_in[1] + R[6] * q_in[2];
            q_out[1] = q_out[1] + R[1] * q_in[0] + R[4] * q_in[1] + R[7] * q_in[2];
            q_out[2] = q_out[2] + R[2] * q_in[0] + R[5] * q_in[1] + R[8] * q_in[2];
        }
    } else if (n_dim == 2) {
        if (transpose == 0) {
            q_out[0] = q_out[0] + R[0] * q_in[0] + R[1] * q_in[1];
            q_out[1] = q_out[1] + R[2] * q_in[0] + R[3] * q_in[1];
        } else {
            q_out[0] = q_out[0] + R[0] * q_in[0] + R[2] * q_in[1];
            q_out[1] = q_out[1] + R[1] * q_in[0] + R[3] * q_in[1];
        }
    } else {
        for (int j=0; j < n_dim; j++)
            q_out[j] = q_out[j] + q_in[j];
    }
}


void apply_shift_rotate(double *q_in, double *q0, double *R, int n_dim,
                        int transpose, double *q_out) {
    double *tmp;
    int j;

    tmp = (double*) malloc(n_dim * sizeof(double));

    // Shift to the specified origin
    for (j=0; j < n_dim; j++) {
        tmp[j] = q_in[j] - q0[j];
    }

    // Apply rotation matrix
    apply_rotate(&tmp[0], R, n_dim, transpose, q_out);

    free(tmp);
}


double c_potential(CPotential *p, double t, double *qp) {
    double v = 0;
    int i, j;
    double *qp_trans;
    qp_trans = (double*) malloc(p->n_dim * sizeof(double));

    for (i=0; i < p->n_components; i++) {
        for (j=0; j < p->n_dim; j++)
            qp_trans[j] = 0.;
        apply_shift_rotate(qp, (p->q0)[i], (p->R)[i], p->n_dim, 0,
                           &qp_trans[0]);
        v = v + (p->value)[i](t, (p->parameters)[i], &qp_trans[0], p->n_dim);
    }
    free(qp_trans);

    return v;
}


double c_density(CPotential *p, double t, double *qp) {
    double v = 0;
    int i, j;
    double *qp_trans;
    qp_trans = (double*) malloc(p->n_dim * sizeof(double));

    for (i=0; i < p->n_components; i++) {
        for (j=0; j < p->n_dim; j++)
            qp_trans[j] = 0.;
        apply_shift_rotate(qp, (p->q0)[i], (p->R)[i], p->n_dim, 0,
                           &qp_trans[0]);
        v = v + (p->density)[i](t, (p->parameters)[i], &qp_trans[0], p->n_dim);
    }
    free(qp_trans);

    return v;
}


void c_gradient(CPotential *p, double t, double *qp, double *grad) {
    int i, j;
    double *qp_trans;
    double *tmp_grad;
    qp_trans = (double*) malloc(p->n_dim * sizeof(double));
    tmp_grad = (double*) malloc(p->n_dim * sizeof(double));

    for (i=0; i < p->n_dim; i++) {
        grad[i] = 0.;
        tmp_grad[i] = 0.;
        qp_trans[i] = 0.;
    }

    for (i=0; i < p->n_components; i++) {
        for (j=0; j < p->n_dim; j++) {
            tmp_grad[j] = 0.;
            qp_trans[j] = 0.;
        }

        apply_shift_rotate(qp, (p->q0)[i], (p->R)[i], p->n_dim, 0,
                           &qp_trans[0]);
        (p->gradient)[i](t, (p->parameters)[i], &qp_trans[0], p->n_dim,
                         &tmp_grad[0]);

        apply_rotate(&tmp_grad[0], (p->R)[i], p->n_dim, 1, &grad[0]);
    }
    free(qp_trans);
    free(tmp_grad);
}


void c_hessian(CPotential *p, double t, double *qp, double *hess) {
    int i;
    double *qp_trans;
    qp_trans = (double*) malloc(p->n_dim * sizeof(double));

    for (i=0; i < pow(p->n_dim,2); i++) {
        hess[i] = 0.;

        if (i < p->n_dim) {
            qp_trans[i] = 0.;
        }
    }

    for (i=0; i < p->n_components; i++) {
        apply_shift_rotate(qp, (p->q0)[i], (p->R)[i], p->n_dim, 0,
                           &qp_trans[0]);
        (p->hessian)[i](t, (p->parameters)[i], &qp_trans[0], p->n_dim, hess);
        // TODO: here - need to apply inverse rotation to the Hessian!
        // - Hessian calculation for potentials with rotations are disabled
    }

    free(qp_trans);
}


double c_d_dr(CPotential *p, double t, double *qp, double *epsilon) {
    double h, r, dPhi_dr;
    int j;
    double r2 = 0;

    for (j=0; j<p->n_dim; j++) {
        r2 = r2 + qp[j]*qp[j];
    }

    // TODO: allow user to specify fractional step-size
    h = 1E-4;

    // Step-size for estimating radial gradient of the potential
    r = sqrt(r2);

    for (j=0; j < (p->n_dim); j++)
        epsilon[j] = h * qp[j]/r + qp[j];

    dPhi_dr = c_potential(p, t, epsilon);

    for (j=0; j < (p->n_dim); j++)
        epsilon[j] = h * qp[j]/r - qp[j];

    dPhi_dr = dPhi_dr - c_potential(p, t, epsilon);

    return dPhi_dr / (2.*h);
}


double c_d2_dr2(CPotential *p, double t, double *qp, double *epsilon) {
    double h, r, d2Phi_dr2;
    int j;
    double r2 = 0;
    for (j=0; j<p->n_dim; j++) {
        r2 = r2 + qp[j]*qp[j];
    }

    // TODO: allow user to specify fractional step-size
    h = 1E-2;

    // Step-size for estimating radial gradient of the potential
    r = sqrt(r2);

    for (j=0; j < (p->n_dim); j++)
        epsilon[j] = h * qp[j]/r + qp[j];
    d2Phi_dr2 = c_potential(p, t, epsilon);

    d2Phi_dr2 = d2Phi_dr2 - 2.*c_potential(p, t, qp);

    for (j=0; j < (p->n_dim); j++)
        epsilon[j] = h * qp[j]/r - qp[j];
    d2Phi_dr2 = d2Phi_dr2 + c_potential(p, t, epsilon);

    return d2Phi_dr2 / (h*h);
}


double c_mass_enclosed(CPotential *p, double t, double *qp, double G,
                       double *epsilon) {
    double r2, dPhi_dr;
    int j;

    r2 = 0;
    for (j=0; j<p->n_dim; j++) {
        r2 = r2 + qp[j]*qp[j];
    }
    dPhi_dr = c_d_dr(p, t, qp, epsilon);
    return fabs(r2 * dPhi_dr / G);
}
