#include "MyModel.h"
#include "DNest4/code/DNest4.h"
#include "Data.h"
#include "Lookup.h"
#include <cmath>

using namespace std;
using namespace DNest4;

MyModel::MyModel()
:objects(8, 100, false, MyConditionalPrior(
	Data::get_instance().get_x_min(), Data::get_instance().get_x_max(),
	Data::get_instance().get_y_min(), Data::get_instance().get_y_max()), PriorType::log_uniform)
,image(Data::get_instance().get_ni(),
	vector<double>(Data::get_instance().get_nj()))
{

}

void MyModel::from_prior(RNG& rng)
{
	objects.from_prior(rng);
	calculate_image();
	sigma = exp(log(1.) + log(1E6)*rng.rand());
}

void MyModel::calculate_image()
{
	// Get coordinate stuff from data
	const vector< vector<double> >& x = Data::get_instance().get_x_rays();
	const vector< vector<double> >& y = Data::get_instance().get_y_rays();

	// Diff
	bool update = objects.get_removed().size() == 0;

	// Components
	const vector< vector<double> >& components = (update)?(objects.get_added())
                                                 :(objects.get_components());
	if(!update)
	{
		// Zero the image
		image.assign(Data::get_instance().get_ni(),
			vector<double>(Data::get_instance().get_nj(), 0.));
	}

	double xc, yc, M, w, q, theta, cos_theta, sin_theta, wsq;
	double rinner, rinnersq, frac;
	double xx, yy, rsq, qInv, coeff1, coeff2;
    double lim1, lim2, tau1, tau2;

	for(size_t k=0; k<components.size(); ++k)
	{
		xc = components[k][0]; yc = components[k][1];
		M = components[k][2]; w = components[k][3];
		q = components[k][4]; theta = components[k][5];
		cos_theta = cos(theta); sin_theta = sin(theta);
		wsq = w*w;
		rinner = components[k][6]*w;
		rinnersq = rinner*rinner;
		frac = components[k][7];
        qInv = 1.0/q;
        coeff1 = (1.0 - frac)*M/(2*M_PI*wsq);
        coeff2 = frac*M/(2*M_PI*rinnersq);
        lim1 = 25.0*wsq;
        lim2 = 25.0*rinnersq;
        tau1 = 1.0/wsq;
        tau2 = 1.0/rinnersq;

		for(size_t i=0; i<image.size(); i++)
		{
			for(size_t j=0; j<image[i].size(); j++)
			{
				xx =  (x[i][j] - xc)*cos_theta + (y[i][j] - yc)*sin_theta;
				yy = -(x[i][j] - xc)*sin_theta + (y[i][j] - yc)*cos_theta;
				rsq = q*xx*xx + yy*yy*qInv;
				// Outer gaussian
				if(rsq < lim1)
					image[i][j] += coeff1*Lookup::evaluate(0.5*rsq*tau1);
				// Inner gaussian
				if(rsq < lim2)
					image[i][j] += coeff2*Lookup::evaluate(0.5*rsq*tau2);
			}
		}
	}
}

double MyModel::perturb(RNG& rng)
{
	double logH = 0.;

	if(rng.rand() <= 0.75)
	{
		logH += objects.perturb(rng);
		calculate_image();
	}
	else
	{
		sigma = log(sigma);
		sigma += log(1E6)*rng.randh();
		sigma = mod(sigma - log(1.), log(1E6)) + log(1.);
		sigma = exp(sigma);
	}

	return logH;
}

double MyModel::log_likelihood() const
{
	const vector< vector<double> >& data = Data::get_instance().get_image();
	const vector< vector<double> >& sig = Data::get_instance().get_sigma();

	double logL = 0.;
	double var;
	for(size_t i=0; i<data.size(); i++)
	{
		for(size_t j=0; j<data[i].size(); j++)
		{
			var = sigma*sigma + sig[i][j]*sig[i][j];
			logL += -0.5*log(2.*M_PI*var)
				-0.5*pow(data[i][j] - image[i][j], 2)/var;
		}
	}

	return logL;
}

void MyModel::print(std::ostream& out) const
{
	out<<setprecision(6);
	for(size_t i=0; i<image.size(); i++)
		for(size_t j=0; j<image[i].size(); j++)
			out<<image[i][j]<<' ';

	out<<setprecision(10);
	objects.print(out); out<<' ';
	out<<sigma<<' ';
}

string MyModel::description() const
{
	return string("");
}

